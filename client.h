#ifndef MY_CLIENT
#define MY_CLIENT
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include "manifest.h"
#define RECV_WIN 		8
#define RECV_WIN_BYTE 	4096
#define CONSUME_RATE	1000
#define READ_DURATION	50
int32_t try = 0;
FILE *out;
int32_t seqNo = 1, ackNo = 0;
int32_t curPack = 1, base = 1;
int32_t recvWin = RECV_WIN, recvWinByte = RECV_WIN_BYTE;

pthread_mutex_t dataMutex = PTHREAD_MUTEX_INITIALIZER;
struct RecvArg {
	int32_t sock;
	struct sockaddr_in serAdd;
};
pthread_t recvDataThread, consumeDataThread;

struct sockaddr_in serAdd, lisAdd;
int32_t sock, addLen, i;
struct sigaction act;
char data[DATLEN];
struct MyPacket sendPack, recvPack;

int32_t checkCreateDataThread = -1;
int32_t isTransferComplete = 0;

char *clientName;
char *fileName;

void connectToServer(int32_t sock, struct sockaddr_in addr, struct sockaddr_in lisAdd, struct MyPacket sendPack, struct MyPacket recvPack);
void cliTerminate(int32_t sock, struct sockaddr_in addr, struct MyPacket sendPack, struct MyPacket recvPack);
void sendFirstHandShakeToListenSocket(int32_t sock, struct sockaddr_in lisAdd);
void catchAlarm(int32_t signal);
void *receiveData(void *arg);
void *consumeData();

#endif