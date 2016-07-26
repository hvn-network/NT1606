#ifndef MY_SERVER
#define MY_SERVER
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include "manifest.h"
int32_t isTimeout = 0, try = 0;
int32_t packetCount;
char* downloadFilename;

void catchAlarm(int32_t signal);
void serHandShake(int32_t sock, struct sockaddr_in addr, struct MyPacket sendPack, struct MyPacket recvPack);
void serTerminate(int32_t sock, struct sockaddr_in addr, struct MyPacket sendPack, struct MyPacket recvPack);
void updateDataArray(int32_t base, FILE *in, char dataArray[WINSIZE][DATLEN]);
void sendDataToClient(int32_t sock, struct sockaddr_in cliAdd);
int32_t listenToConnection(int32_t lisSock, struct sockaddr_in lisAdd, struct sockaddr* cliAdd);
int32_t my_min(int32_t a, int32_t b);
#endif