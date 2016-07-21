
#include "server.h"

/* server listen to client connection, fork new process to send data to the client */

int main() {
	FILE *in;
	struct sockaddr_in serAdd, cliAdd, lisAdd;
	int sock, lisSock;
	struct sigaction act;
	pid_t pid;
	srand (time(NULL));  //set a random

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
	lisAdd.sin_addr.s_addr 	= INADDR_ANY;

	if (bind(lisSock, (struct sockaddr*)&lisAdd, sizeof(lisAdd)) < 0) {
		perror("bind failed");
		exit(0);
	}
	puts("Waiting for connection...");

	//listen and send data to client
	while (listenToConnection(lisSock, lisAdd, (struct sockaddr*) &cliAdd)) {
		pid = fork();
		if (pid != 0) {
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

	}
	close(sock);
	return 0;
}
void serHandShake(int sock, struct sockaddr_in addr, struct MyPacket sendPack, struct MyPacket recvPack) {
	int addLen = sizeof(addr);
	try = 0;
	//Step 1: receive first syn signal (SYN) from client
	//this is done by listen

	//Step 2: send SYN_ACK
sendSYN_ACK:

	puts("---received SYN");
	preparePack(&sendPack, 0, 0, 0, SYN_ACK, "");
	puts("---send SYN_ACK");
	setTimer(TIMEOUT);
	if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*)&addr, addLen) < 0) {
		perror("send failed");
		exit(1);
	}


	//Step 3: receive SYN_ACK_ACK
	while (recvfrom(sock, &recvPack, sizeof(recvPack), 0, (struct sockaddr*)&addr, &addLen) < 0) {
		if (try < 1) {
				puts("============================time out=============================");
				printf("---resend time: %d\n", try);
				goto sendSYN_ACK;
			}
		else {
			puts("connection corrupted");
			exit(1);
		}
	}
	resetTimer(&try);

	if (recvPack.type == SYN_ACK_ACK) {
		puts(recvPack.data);
		puts("---received SYN_ACK_ACK. \n---Finish handshaking!");
		puts("---prepare send data to the client...");
	} else {
		puts("something happened");
	}

	//return 0;
}

void serTerminate(int sock, struct sockaddr_in addr, struct MyPacket sendPack, struct MyPacket recvPack) {
	int addLen = sizeof(addr);
	int i;
	//send FIN
	puts("end of file. Prepare to terminate...");
	preparePack(&sendPack, 0, 0, 0, FIN, "");

serverSendFIN:
	puts("---send FIN");
	setTimer(TIMEOUT);
	if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*)&addr, addLen) < 0) {
		perror("send failed");
		exit(0);
	}
	//receive FIN_ACK (wait until receive all acks of the last frame)
serverReceiveFIN_ACK:
	while (recvfrom(sock, &recvPack, sizeof(recvPack), 0, (struct sockaddr*)&addr, &addLen) < 0) {
		if (try < MAXTRIES) {
				puts("============================time out=============================");
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
		puts("---received FIN_ACK");
	} else {
		puts("received an ACK");
		goto serverSendFIN;
	}

serverReceiveFIN:
	setTimer(TIMEOUT);
	while (recvfrom(sock, &recvPack, sizeof(recvPack), 0, (struct sockaddr*)&addr, &addLen) < 0) {
		if (try < MAXTRIES) {
				puts("============================time out=============================");
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
		puts("---received FIN");

	} else {
		puts("received something wrong");
		goto serverReceiveFIN;
	}

	//send FIN_ACK
	preparePack(&sendPack, 0, 0, 0, FIN_ACK, "");
	puts("---send FIN_ACK");
	for (i = 0; i < MAXTRIES; i++) {
		if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*)&addr, addLen) < 0) {
			perror("send failed");
			exit(0);
		}
	}



	puts("---Terminate successful!");

}
void catchAlarm(int signal) {
	isTimeout = 1;
	try++;
}

void updateDataArray(int base, FILE *in, char dataArray[WINSIZE][DATLEN]) { //, int recvWinSize) {
	//shift data in TCP based on BASE
	int i;
	int distance = packetCount + 1 - base;
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

int listenToConnection(int lisSock, struct sockaddr_in lisAdd, struct sockaddr* cliAdd) {
	struct MyPacket recvPack;
	int addLen = sizeof(lisAdd);

	//listen to SIN signal from client
	if (recvfrom(lisSock, &recvPack, sizeof(struct MyPacket), 0, cliAdd, &addLen) < 0) {
		perror("recv failed");
		exit(0);
	}
	if (recvPack.type == SYN) {
		puts("OK");
		return 1;

	} else {
		return 0;
	}

}

void sendDataToClient(int sock, struct sockaddr_in cliAdd) {
	FILE *in;
	char data[DATLEN];
	struct MyPacket sendPack, recvPack;
	char dataArray[WINSIZE][DATLEN]; 			//store array for retransmission
	int i, addLen;
	int seqNo = 1, ackNo = 1;
	int curPack = 1, base = 1;
	int recvWinSize = 1;
	addLen = sizeof(cliAdd);
	int currentType = DATA;

	//open file and readline
	in = fopen("file.in", "rt");
	//handshake
	serHandShake(sock, cliAdd, sendPack, recvPack);

	//begin sending data
	while (1) {
		if (currentType == DATA) {
			//send packet
			printf("---current base: %d\n", base);

			//if timeout resend current frame
			if (isTimeout) {
				setTimer(TIMEOUT);
				isTimeout = 0;
				printf("resend from seqNo: %d \n", base);
				for (i = 0; i < min(WINSIZE, recvWinSize); i++) {
					printf("---send packet: %d\n", base + i);
					preparePack(&sendPack, base + i, 0, 0, DATA, dataArray[i]);
					if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*) &cliAdd, addLen) < 0) {
						perror("send failed");
						exit(0);
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
				while (seqNo < base + min(WINSIZE, recvWinSize)) {
					if (seqNo == base) {
						setTimer(TIMEOUT);
					}
					//memset(data, '\0', DATLEN);
					//fgets(data, DATLEN, in);
					printf("---send packet: %d\n", seqNo);
					preparePack(&sendPack, seqNo, 0, 0, DATA, dataArray[seqNo - base]);
					if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*) &cliAdd, addLen) < 0) {
						perror("send failed");
						exit(0);
					}
					seqNo++;
				}
			}
		} else if (currentType == PROBE) {
			puts("---send probe");
			preparePack(&sendPack, 0, 0, 0, PROBE, "");
			setTimer(TIMEOUT);
			if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*) &cliAdd, addLen) < 0) {
				perror("send failed");
				exit(0);
			}

		}

		//receive ack
		int recvStatus;
		if ((recvStatus = recvfrom(sock, &recvPack, sizeof(recvPack), 0, (struct sockaddr*) &cliAdd, &addLen)) < 0) {
			puts("============================time out=============================");
			printf("---resend time: %d \n", try);
		}

		//check receive status
		if (recvStatus >= 0) {
			resetTimer(&try);
			//check recvWinsize
			if (recvPack.winSize > 0) {
				//update base if received appropriate ACK
				if ((recvPack.ackNo >= base) && (dataArray[min(WINSIZE, recvWinSize) - 1][0] != '*' && dataArray[min(WINSIZE, recvWinSize) - 1][0] != 0)) {
					//cumulative ack technique
					base += (recvPack.ackNo - base + 1);
				}
				recvWinSize = recvPack.winSize;
				currentType = DATA;
			} else {
				currentType = PROBE;
			}
			printf("---recieved ACK: %d\n", recvPack.ackNo);
			printf("---current recv win size: %d %d\n", recvPack.winSize, isTimeout);

			if (seqNo == base) {
				resetTimer(&try);
			} else {
				setTimer(TIMEOUT);
			}
		}

		puts("_____________________________________________");


		//terminate if end of file and received all the packets
		if ((dataArray[min(WINSIZE, recvWinSize) - 1][0] == '*' || dataArray[min(WINSIZE, recvWinSize) - 1][0] == 0) && recvPack.ackNo == seqNo - 1) {
			serTerminate(sock, cliAdd, sendPack, recvPack);
			break;
		}


	}
	//reset timer
	resetTimer(&try);
	fclose(in);
}

int min(int a, int b) {
	if (a > b)
		return b;
	return a;
}
