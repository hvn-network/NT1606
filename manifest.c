#include "manifest.h"
#include <string.h>
#include <stdio.h>
int32_t preparePack(struct MyPacket *packet, int32_t seqNo, int32_t ackNo, int32_t winSize, int32_t type, char data[])
{
    memset(packet, 0, sizeof(packet));
    if (CONVERT_SYSTEM) {
        packet->seqNo   = convertBigAndLitteEndian(seqNo);
        packet->ackNo   = convertBigAndLitteEndian(ackNo);
        packet->winSize = convertBigAndLitteEndian(winSize);
        packet->type    = convertBigAndLitteEndian(type);
    } else {
        packet->seqNo   = (seqNo);
        packet->ackNo   = (ackNo);
        packet->winSize = (winSize);
        packet->type    = (type);
    }
    strcpy(packet->data, data);
    return 0;
}

int32_t sendUnreliably(int32_t socket, struct MyPacket *packet, int32_t flag, struct sockaddr *address, int32_t addLen) {
    int32_t randnum = rand() % 100 + 1;
    usleep(PROPA_DELAY * 1000);
    if (LOSSRATE < randnum) {
        if (sendto(socket, packet, sizeof(struct MyPacket), flag, address, addLen) < 0) {
            return -1;
        }
        //printf("package sent\n");
        return 1;
    } else {
        puts("             PACKAGE LOSS");
        return 0;
    }
}
void setTimer(int32_t timeout)
{
    alarm(timeout);
}
void resetTimer(int32_t *trytime) {

    *trytime = 0;
    alarm(0);
}

int32_t convertBigAndLitteEndian(int32_t num) {
    int32_t swapped;
    swapped = ((num >> 24) & 0xff) | // move byte 3 to byte 0
              ((num << 8) & 0xff0000) | // move byte 1 to byte 2
              ((num >> 8) & 0xff00) | // move byte 2 to byte 1
              ((num << 24) & 0xff000000); // byte 0 to byte 3
    return swapped;
}

void showTimeAndMsg(char *msg) {
    gettimeofday(curTimeMS, NULL);
    int duration = (int)(((unsigned long long)(curTimeMS->tv_sec) * 1000 +
                          (unsigned long long)(curTimeMS->tv_usec) / 1000) -
                         ((unsigned long long)(startTimeMS->tv_sec) * 1000 +
                          (unsigned long long)(startTimeMS->tv_usec) / 1000));

    printf("[%8.2f] %s\n", ((float)duration)/1000, msg);
}

void showTimeStamp(){
    gettimeofday(curTimeMS, NULL);
    int duration = (int)(((unsigned long long)(curTimeMS->tv_sec) * 1000 +
                          (unsigned long long)(curTimeMS->tv_usec) / 1000) -
                         ((unsigned long long)(startTimeMS->tv_sec) * 1000 +
                          (unsigned long long)(startTimeMS->tv_usec) / 1000));
    printf("[%8.2f] ",((float)duration)/1000);
}