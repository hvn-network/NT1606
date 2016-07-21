#ifndef MANIFEST
#define MANIFEST
#include <arpa/inet.h>

#define LIS_PORT 		6666
#define DATLEN			495	//data length in bytes
#define TIMEOUT 		1	//time out in seconds
#define MAXTRIES		5	//number of try times before corrupting
#define SERVER			"127.0.0.1"

#define SYN 			1	//
#define SYN_ACK 		2	// Used for handshake
#define SYN_ACK_ACK 	3	//
#define ACK 			4
#define DATA 			5
#define FIN				6
#define FIN_ACK			7
#define PROBE			8
#define PROBE_ACK		9

#define WINSIZE 		8
#define LOSSRATE		40	//percentage of lossrate
#define PROPA_DELAY		100	//propagation delay in ms
#define PROBE_SIZE		16	//probe size in bytes


//packet prototype
struct MyPacket {
	int seqNo;
	int ackNo;
	int winSize;
	int type;
	char data[495];
};

//simulate the channel with propagation delay and packet loss
int sendUnreliably(int socket, struct MyPacket *packet, int flag, struct sockaddr *address, int addLen);

//read info (seqNo, ackNo, ...) into packet
int preparePack(struct MyPacket *packet, int seqNo, int ackNo, int winSize, int type, char data[]);

//handle timer
void setTimer(int timeout);
void resetTimer(int *trytime);
#endif 