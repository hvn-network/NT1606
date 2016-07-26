#include "client.h"


/* the client has two main thread, the first one receive data and check if it is desirable, the other consume data */

int32_t main(int32_t argc, char* argv[]) {

	if (argc != 3) {
		puts("USAGE: ./client CLIENTNAME FILENAME");
		exit(1);
	} else {
		clientName = (char *) malloc (sizeof(clientName));
		fileName   = (char *) malloc (sizeof(fileName));
		strcpy(clientName, argv[1]);
		strcpy(fileName, argv[2]);
	}

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

	/* init system timestamp */
	startTimeMS = (struct timeval*)malloc(sizeof(struct timeval));
	curTimeMS 	= (struct timeval*)malloc(sizeof(struct timeval));
	gettimeofday(startTimeMS, NULL);


	//create socket
	if ( (sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("socket failed");
		exit(1);
	}


	//assign listion port and IP
	memset((char*) &lisAdd, 0, sizeof(lisAdd));
	lisAdd.sin_family		= AF_INET;
	lisAdd.sin_port			= htons(LIS_PORT);
	if (inet_aton(SERVER, &lisAdd.sin_addr) == 0) {
		perror("inet_aton failed");
		exit(0);
	}

	out = fopen(clientName, "w");

	struct RecvArg *arg;
	arg = (struct RecvArg*) malloc(sizeof(struct RecvArg));
	arg -> sock 	= sock;
	arg -> serAdd 	= serAdd;

	//handshake
	connectToServer(sock, serAdd, lisAdd, sendPack, recvPack);

	checkCreateDataThread =  pthread_create(&recvDataThread, NULL, receiveData, (void *) arg);
	pthread_create(&consumeDataThread, NULL, consumeData, NULL);


	pthread_join(recvDataThread, NULL);
	pthread_join(consumeDataThread, NULL);

	cliTerminate(sock, serAdd, sendPack, recvPack);
	fclose(out);
	close(sock);
	return 0;
}

void sendFirstHandShakeToListenSocket(int32_t sock, struct sockaddr_in lisAdd) {
	struct MyPacket sendPack;
	int32_t addLen = sizeof(lisAdd);
	preparePack(&sendPack, 1, 2, 3, 4, "abc");

	if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*) &lisAdd, addLen) < 0) {
		perror("send failed 1");
		exit(0);
	}
	setTimer(TIMEOUT);
}

void connectToServer(int32_t sock, struct sockaddr_in addr, struct sockaddr_in lisAdd, struct MyPacket sendPack, struct MyPacket recvPack) {
	int32_t addLen = sizeof(addr);
	try = 0;
	//Step 1: send first syn signal (SYN) to server (!this will be done through listen socket)
sendSYN:
	showTimeAndMsg("Sending SYN");
	preparePack(&sendPack, 0, 0, 0, SYN, "");
	setTimer(TIMEOUT);
	if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*) &lisAdd, addLen) < 0) {
		perror("send failed");
		exit(1);
	}


	//Step 2: receive SYN_ACK
	while (recvfrom(sock, &recvPack, sizeof(recvPack), 0, (struct sockaddr*)&addr, &addLen) < 0) {
		if (try < MAXTRIES) {
				puts("=========================== TIME OUT =============================");
				showTimeStamp();
				printf("Resend time: %d\n", try);
				goto sendSYN;
			}
		else {
			puts("conection corrupted");
			exit(1);
		}
	}
	resetTimer(&try);

	//Step 3: send SYN_ACK_ACK
sendSYN_ACK_ACK:
	if (recvPack.type == SYN_ACK) {
		showTimeAndMsg("Received SYN_ACK");
		showTimeAndMsg("Send SYN_ACK_ACK");
		preparePack(&sendPack, 0, 0, 0, SYN_ACK_ACK, fileName);
		if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*)&addr, addLen) < 0) {
			perror("send failed");
			exit(1);
		}
		setTimer(TIMEOUT);
	}

	//if receive data
	while (recvfrom(sock, &recvPack, sizeof(recvPack), 0, (struct sockaddr*)&addr, &addLen) < 0) {
		if (try < MAXTRIES) {
				puts("============================time out=============================");
				showTimeStamp();
				printf("Resend time: %d\n", try);
				goto sendSYN_ACK_ACK;
			}
		else {
			puts("conection corrupted");
			exit(1);
		}
	}
	resetTimer(&try);
	//send ack if receive data
	if (recvPack.type == DATA) {
		if (recvPack.seqNo == ackNo + 1) {
			ackNo++;
			showTimeAndMsg(recvPack.data);
			fputs(recvPack.data, out);

		}

		preparePack(&sendPack, 0, ackNo, RECV_WIN - 1, ACK, "");
		showTimeStamp();
		printf("send ack %d\n", ackNo);
		if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*) &addr, addLen) < 0) {
			perror("send failed");
			exit(1);
		}
	}
}

void cliTerminate(int32_t sock, struct sockaddr_in addr, struct MyPacket sendPack, struct MyPacket recvPack) {
	int32_t addLen = sizeof(addr);

	showTimeAndMsg("Received FIN from server");

	//send FIN ACK
clientSendFIN_ACK:
	showTimeAndMsg("Sending FIN_ACK");

	preparePack(&sendPack, 0, 0, 0, FIN_ACK, "");

	setTimer(TIMEOUT);
	if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*)&addr, addLen) < 0) {
		perror("send failed");
		exit(1);
	}

	//send FIN
	showTimeAndMsg("Sending FIN to server");
	preparePack(&sendPack, 0, 0, 0, FIN, "");
	setTimer(TIMEOUT);
	if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*)&addr, addLen) < 0) {
		perror("send failed");
		exit(1);
	}

	//receive FIN_ACK
clientReceiveFIN_ACK:

	while (recvfrom(sock, &recvPack, sizeof(recvPack), 0, (struct sockaddr*)&addr, &addLen) < 0) {
		if (try < MAXTRIES) {
				puts("============================time out=============================");
				showTimeStamp();
				printf("Resend time: %d\n", try);
				goto clientSendFIN_ACK;
			}
		else {
			puts("connection corrupted");
			exit(1);
		}
	}
	resetTimer(&try);

	if (recvPack.type == FIN_ACK) {
		showTimeAndMsg("Received FIN_ACK!");
		showTimeAndMsg("Terminate successful");
	} else {
		puts("something happened!!!");
		goto clientSendFIN_ACK;
	}
}

void catchAlarm(int32_t signal) {
	try++;
	//kill process if timeout occurs during transfering data
	if (checkCreateDataThread == 0 && isTransferComplete == 0) {
		showTimeAndMsg("Connection corrupted. Killing process!!!");
		pthread_kill(recvDataThread, SIGKILL);
	}
}

void *receiveData(void *arg) {
	sock 					= ((struct RecvArg*) arg) -> sock;
	serAdd 					= ((struct RecvArg*) arg) -> serAdd;
	int32_t addLen 					= sizeof(serAdd);
	struct MyPacket recvPack, sendPack;
	//receive packet
	while (1) {

		setTimer(TIMEOUT * MAXTRIES);
		if (recvfrom(sock, &recvPack, sizeof(recvPack), 0, (struct sockaddr*) &serAdd, &addLen) < 0) {

			if (errno == EINTR) {
				showTimeAndMsg("Connection corrupted");
				puts("**************************************************");
			} else
				perror("recv failed");
			exit(1);
		}
		resetTimer(&try);

		//terminate if received terminate signal
		if (recvPack.type == FIN) {
			isTransferComplete = 1;

			break;
		} else if (recvPack.type == DATA) {
			showTimeStamp();
			printf("Receive packet: %d\n", recvPack.seqNo);
			//update ackNo if receive expected seqNo
			if (recvPack.seqNo == ackNo + 1) {

				//update windowSize
				pthread_mutex_lock(&dataMutex);
				recvWinByte -= DATLEN;
				if (recvWinByte < 0) {
					recvWinByte += DATLEN;
				}
				recvWin = recvWinByte / DATLEN;
				pthread_mutex_unlock(&dataMutex);

				//put data
				ackNo++;
				showTimeAndMsg(recvPack.data);
				fputs(recvPack.data, out);

			}
			showTimeStamp();
			printf("send ack %d\n", ackNo);
			preparePack(&sendPack, 0, ackNo, recvWin, ACK, "");
			if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*) &serAdd, addLen) < 0) {
				perror("send failed");
				exit(1);
			}


		} else if (recvPack.type == PROBE) {
			showTimeAndMsg("received PROBE");
			//update window size and send
			pthread_mutex_lock(&dataMutex);
			recvWinByte -= PROBE_SIZE;
			if (recvWinByte < 0) {
				recvWinByte += PROBE_SIZE;
			}
			recvWin = recvWinByte / DATLEN;
			pthread_mutex_unlock(&dataMutex);
			showTimeAndMsg("Send PROBE_ACK");
			preparePack(&sendPack, 0, 0, recvWin, PROBE_ACK, "");
			if (sendUnreliably(sock, &sendPack, 0, (struct sockaddr*) &serAdd, addLen) < 0) {
				perror("send failed");
				exit(1);
			}

		}
		puts("_____________________________________________");
	}
}

void *consumeData() {
	int32_t consumedData;
	while (1) {
		if (!isTransferComplete && (recvWinByte < RECV_WIN_BYTE)) {
			consumedData = (int)(CONSUME_RATE * READ_DURATION / 1000);
			pthread_mutex_lock(&dataMutex);
			recvWinByte += consumedData;
			if (recvWinByte > RECV_WIN_BYTE) {
				recvWinByte = RECV_WIN_BYTE;
			}
			pthread_mutex_unlock(&dataMutex);
			usleep(READ_DURATION * 1000);

		}
		else if (isTransferComplete) {
			break;
		}

	}
}


