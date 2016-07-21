#ifndef MY_SERVER
#define MY_SERVER
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include "manifest.h"
int isTimeout = 0, try = 0;
int packetCount;

void catchAlarm(int signal);
void serHandShake(int sock, struct sockaddr_in addr, struct MyPacket sendPack, struct MyPacket recvPack);
void serTerminate(int sock, struct sockaddr_in addr, struct MyPacket sendPack, struct MyPacket recvPack);
void updateDataArray(int base, FILE *in, char dataArray[WINSIZE][DATLEN]);
void sendDataToClient(int sock, struct sockaddr_in cliAdd);
int listenToConnection(int lisSock, struct sockaddr_in lisAdd, struct sockaddr* cliAdd);

#endif