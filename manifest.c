#include "manifest.h"
#include <string.h>
#include <stdio.h>
int preparePack(struct MyPacket *packet, int seqNo, int ackNo, int winSize, int type, char data[])
{
    memset(packet, 0, sizeof(packet));
    packet->seqNo = seqNo;
    packet->ackNo = ackNo;
    packet->winSize = winSize;
    packet->type = type;
    strcpy(packet->data, data);
    return 0;
}

int sendUnreliably(int socket, struct MyPacket *packet, int flag, struct sockaddr *address, int addLen) {
    int randnum = rand() % 100 + 1;
    usleep(PROPA_DELAY * 1000);
    if (LOSSRATE < randnum) {
        if (sendto(socket, packet, sizeof(struct MyPacket), flag, address, addLen) < 0) {
            return -1;
        }
        printf("---package sent\n");
        return 1;
    } else {
        printf("---package loss\n");
        return 0;
    }
}
void setTimer(int timeout)
{
    alarm(timeout);
}
void resetTimer(int *trytime) {
    
    *trytime = 0;
    alarm(0);
}