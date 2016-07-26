#ifndef MANIFEST
#define MANIFEST
#include <arpa/inet.h>
#include <sys/time.h>    /* defines the time structure */


#define LIS_PORT 		6666
#define DATLEN			495	//data length in bytes
#define TIMEOUT 		1	//time out in seconds
#define MAXTRIES		5	//number of try times before corrupting
#define CONVERT_SYSTEM	1   //convert B&L endian
#define END_OF_FILE     '*'
#define SERVER			"192.168.1.1" //"192.168.100.249"
//#define SERVER			"127.0.0.1"
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
#define LOSSRATE		5	//percentage of lossrate
#define PROPA_DELAY		100	//propagation delay in ms
#define PROBE_SIZE		16	//probe size in bytes


/* get timestamp in minisecond*/
struct timeval *startTimeMS, *curTimeMS;

//packet prototype
struct MyPacket {
	int32_t seqNo;
	int32_t ackNo;
	int32_t winSize;
	int32_t type;
	char data[495];
};

//simulate the channel with propagation delay and packet loss
int32_t sendUnreliably(int32_t socket, struct MyPacket *packet, int32_t flag, struct sockaddr *address, int32_t addLen);

//read info (seqNo, ackNo, ...) into packet
int32_t preparePack(struct MyPacket *packet, int32_t seqNo, int32_t ackNo, int32_t winSize, int32_t type, char data[]);

//handle timer
void setTimer(int32_t timeout);
void resetTimer(int32_t *trytime);
int32_t convertBigAndLitteEndian(int32_t num);
void showTimeAndMsg(char *msg);

#endif