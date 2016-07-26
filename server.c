
#include "server.h"

/* server listen to client connection, fork new process to send data to the client */

int32_t main() {
	FILE *in;
	struct sockaddr_in serAdd, cliAdd, lisAdd;
	int32_t sock, lisSock;
	struct sigaction act;
	pid_t pid;
	srand ((unsigned int) time(NULL));  //set a random

	/* init system timestamp */
	startTimeMS = (struct timeval*)malloc(sizeof(struct timeval));
	curTimeMS 	= (struct timeval*)malloc(sizeof(struct timeval));


	//signal handler to create timer
	act.sa_handler = catchAlarm;
	if (sigfillset(&act.sa_mask) < 0) {
		perror("sigfillset failed");
		exit(0);
	}
	act.sa_flags = 0;
	if (sigaction(SIGALRM, &act, 0) < 0) {
		perror("sigaction failed");
		exit(0);
	}


	//create socket for listen connections
	if ((lisSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("socket failed");
		exit(0);
	}

	lisAdd.sin_family		= AF_INET;
	lisAdd.sin_port			= htons(LIS_PORT);
	lisAdd.sin_addr.s_addr 	= inet_addr(SERVER);

	if (bind(lisSock, (struct sockaddr*)&lisAdd, sizeof(lisAdd)) < 0) {
		perror("bind failed");
		exit(0);
	}
	puts("Waiting for connection...");

	//listen and send data to client
	int32_t listenStatus;

	do {
		listenStatus = listenToConnection(lisSock, lisAdd, (struct sockaddr*) &cliAdd);
		gettimeofday(startTimeMS, NULL);
		pid = fork();
		if (pid < 0) {
			perror("fork failed");
			exit(1);
		}
		else if (pid != 0) {
			continue;
		}
		else {
			if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
				perror("socket failed");
				exit(0);
			}
			packetCount = 0;
			sendDataToClient(sock, cliAdd);
			puts("*********************************");
		}

	} while (listenStatus > 0);

	puts("finish main func");
	close(sock);
	return 0;
}
void serHandShake(int32_t sock, struct sockaddr_in addr, struct MyPacket sendPack, struct MyPacket recvPack) {
	int32_t addLen = sizeof(addr);
	try = 0;
	//Step 1: receive first syn signal (SYN) from client
	//this is done by listen

	//Step 2: send SYN_ACK
sendSYN_ACK:

	preparePack(&sendPack, 0, 0, 0, SYN_ACK, "");
	showTimeAndMsg("Sending SYN_ACK");
	setTimer(TIMEOUT);
	if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*)&addr, addLen) < 0) {
		perror("send failed");
		exit(1);
	}


	//Step 3: receive SYN_ACK_ACK
	while (recvfrom(sock, &recvPack, sizeof(recvPack), 0, (struct sockaddr*)&addr, &addLen) < 0) {
		if (try < 1) {
				puts("=========================== TIME OUT =============================");
				showTimeStamp();
				printf("resend time: %d\n", try);
				goto sendSYN_ACK;
			}
		else {
			puts("connection corrupted");
			exit(1);
		}
	}
	resetTimer(&try);

	if (recvPack.type == SYN_ACK_ACK) {
		/* get requested filename */
		downloadFilename = (char *)malloc(20 * sizeof(char));
		strcpy(downloadFilename, recvPack.data);
		showTimeAndMsg("received SYN_ACK_ACK.");
		showTimeAndMsg("Finish handshaking!");
		showTimeAndMsg("Prepare send data to the client...");
	} else {
		puts("something happened");
	}

	//return 0;
}

void serTerminate(int32_t sock, struct sockaddr_in addr, struct MyPacket sendPack, struct MyPacket recvPack) {
	int32_t addLen = sizeof(addr);
	int32_t i;
	//send FIN
	showTimeAndMsg("End of file. Prepare to terminate...");
	preparePack(&sendPack, 0, 0, 0, FIN, "");

serverSendFIN:
	showTimeAndMsg("Sending FIN");
	setTimer(TIMEOUT);
	if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*)&addr, addLen) < 0) {
		perror("send failed");
		exit(0);
	}
	//receive FIN_ACK (wait until receive all acks of the last frame)
serverReceiveFIN_ACK:
	while (recvfrom(sock, &recvPack, sizeof(recvPack), 0, (struct sockaddr*)&addr, &addLen) < 0) {
		if (try < MAXTRIES) {
				puts("=========================== TIME OUT =============================");
				showTimeStamp();
				printf("---resend time: %d\n", try);
				goto serverSendFIN;
			}
		else {
			puts("connection corrupted");
			exit(1);
		}
	}
	resetTimer(&try);
	if (recvPack.type == FIN_ACK) {
		showTimeAndMsg("Received FIN_ACK");
	} else {
		showTimeAndMsg("Received an ACK");
		goto serverSendFIN;
	}

serverReceiveFIN:
	setTimer(TIMEOUT);
	while (recvfrom(sock, &recvPack, sizeof(recvPack), 0, (struct sockaddr*)&addr, &addLen) < 0) {
		if (try < MAXTRIES) {
				puts("=========================== TIME OUT =============================");
				printf("---resend time: %d\n", try);
				goto serverReceiveFIN;
			}
		else {
			puts("connection corrupted");
			exit(1);
		}
	}
	resetTimer(&try);
	if (recvPack.type == FIN) {
		showTimeAndMsg("Received FIN");

	} else {
		puts("received something wrong");
		goto serverReceiveFIN;
	}

	//send FIN_ACK
	preparePack(&sendPack, 0, 0, 0, FIN_ACK, "");
	showTimeAndMsg("Sending FIN_ACK");
	for (i = 0; i < MAXTRIES; i++) {
		if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*)&addr, addLen) < 0) {
			perror("send failed");
			exit(0);
		}
	}



	showTimeAndMsg("Terminate successful!");

}
void catchAlarm(int32_t signal) {
	isTimeout = 1;
	try++;
}

void updateDataArray(int32_t base, FILE *in, char dataArray[WINSIZE][DATLEN]) { //, int32_t recvWinSize) {
	//shift data in TCP based on BASE
	int32_t i;
	int32_t distance = packetCount + 1 - base;
	//clear dataArray if base = seqNo
	if (distance == 0) {
		memset(dataArray, 0, sizeof(dataArray));
	} else {
		for (i = 0; i < distance; i++) {
			strcpy(dataArray[i], dataArray[i + WINSIZE - distance]);
		}
	}

	//clear Window from distance
	char temp[DATLEN];
	for (i = distance; i < WINSIZE; i++) {
		strcpy(dataArray[i], "");
	}
	//insert data
	for (i = distance; i < WINSIZE; i++) {
		memset(temp, '\0', DATLEN);
		fgets(temp, DATLEN, in);
		strcpy(dataArray[i], temp);
		packetCount++;
	}
}

int32_t listenToConnection(int32_t lisSock, struct sockaddr_in lisAdd, struct sockaddr* cliAdd) {
	struct MyPacket recvPack;
	int32_t addLen = sizeof(lisAdd);

	//listen to SIN signal from client
	if (recvfrom(lisSock, &recvPack, sizeof(struct MyPacket), 0, cliAdd, &addLen) < 0) {
		perror("recv failed");
		exit(0);
	}
	if (recvPack.type == SYN) {
		printf("[%8.2f] %s\n", 0.00, "Received SYN");
		return 1;

	} else {
		puts("listen error");
		return 0;
	}

}

void sendDataToClient(int32_t sock, struct sockaddr_in cliAdd) {
	FILE *in;
	char data[DATLEN];
	struct MyPacket sendPack, recvPack;
	char dataArray[WINSIZE][DATLEN]; 			//store array for retransmission
	int32_t i, addLen;
	int32_t seqNo = 1, ackNo = 1;
	int32_t curPack = 1, base = 1;
	int32_t recvWinSize = WINSIZE;
	addLen = sizeof(cliAdd);
	int32_t currentType = DATA;

	int isEOF = 0;

	//handshake
	serHandShake(sock, cliAdd, sendPack, recvPack);

	//open file and readline


	in = fopen(downloadFilename, "rt");

	//begin sending data
	while (1) {
		if (currentType == DATA) {
			//send packet
			showTimeStamp();
			printf("Current base: %d\n", base);


			//if timeout resend current frame
			if (isTimeout) {
				setTimer(TIMEOUT);
				isTimeout = 0;
				showTimeStamp();
				printf("Resend from seqNo: %d \n", base);
				for (i = 0; i < my_min(WINSIZE, recvWinSize); i++) {
					showTimeStamp();
					printf("Send packet: %d\n", base + i);
					preparePack(&sendPack, base + i, 0, 0, DATA, dataArray[i]);
					if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*) &cliAdd, addLen) < 0) {
						perror("send failed");
						exit(0);
					}
					if(*dataArray[i] == END_OF_FILE){
						isEOF = 1;
						break;
					}
				}

				// if try ==  maxtries, terminate connection
				if (try == MAXTRIES) {
						puts("Client is corrupted!");
						puts("Terminate connection!");
						break;
					}
			} else {
				try = 0;

				//update array based on BASE
				updateDataArray(base, in, dataArray);

				//send the next seqNo
				while (!isEOF && seqNo < base + my_min(WINSIZE, recvWinSize)) {
					if (seqNo == base) {
						setTimer(TIMEOUT);
					}
					showTimeStamp();
					printf("Send packet: %d\n", seqNo);
					preparePack(&sendPack, seqNo, 0, 0, DATA, dataArray[seqNo - base]);

					if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*) &cliAdd, addLen) < 0) {
						perror("send failed");
						exit(0);
					}

					/* check end of file */ 
					if(*dataArray[seqNo -base] == END_OF_FILE){
						isEOF = 1;
						seqNo++;
						break;
					}
					seqNo++;
				}
			}
		} else if (currentType == PROBE) {
			showTimeAndMsg("Sending probe");
			preparePack(&sendPack, 0, 0, 0, PROBE, "");
			setTimer(TIMEOUT);
			if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*) &cliAdd, addLen) < 0) {
				perror("send failed");
				exit(0);
			}

		}

		//receive ack
		int32_t recvStatus;
		if ((recvStatus = recvfrom(sock, &recvPack, sizeof(recvPack), 0, (struct sockaddr*) &cliAdd, &addLen)) < 0) {
			puts("=========================== TIME OUT =============================");
			showTimeStamp();
			printf("Resending time: %d \n", try);
		}

		//check receive status
		if (recvStatus >= 0) {
			resetTimer(&try);
			//check recvWinsize
			if (recvPack.winSize > 0) {
				//update base if received appropriate ACK
				if ((recvPack.ackNo >= base) && (dataArray[my_min(WINSIZE, recvWinSize) - 1][0]) != END_OF_FILE) {
					//cumulative ack technique
					base += (recvPack.ackNo - base + 1);
				}
				recvWinSize = recvPack.winSize;
				currentType = DATA;
			} else {
				currentType = PROBE;
			}
			showTimeStamp();
			printf("Recieved ACK: %d\n", recvPack.ackNo);
			showTimeStamp();
			printf("Current recv win size: %d\n", recvPack.winSize);

			if (seqNo == base) {
				resetTimer(&try);
			} else {
				setTimer(TIMEOUT);
			}
		}

		puts("_____________________________________________");


		
		if (isEOF && recvPack.ackNo == seqNo - 1) {
			serTerminate(sock, cliAdd, sendPack, recvPack);
			break;
		}



	}
	//reset timer
	resetTimer(&try);
	fclose(in);
}

int32_t my_min(int32_t a, int32_t b) {
	if (a > b)
		return b;
	return a;
}
