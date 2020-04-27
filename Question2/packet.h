#ifndef MESSAGEDEF_H
#define MESSAGEDEF_H

#define PACKET_SIZE 20
#define SERVER_PORT1 8492
#define RELAY_PORT1 8534
#define RELAY_PORT2 8543
#define SERVER_PORT2 8672
#define WINDOWSIZE 10
typedef struct{
    int payloadSize;
    unsigned int seqNo;
    int lastPacket:1;
    int dataPacket:1;
    char payload[PACKET_SIZE];
} Packet;

#define SETNOTLASTPACKET(p) p.lastPacket=0;
#define SETLASTPACKET(p)    p.lastPacket=1;
#define SETDATAPACKET(p)    p.dataPacket=1;
#define SETACKPACKET(p)     p.dataPacket=0;
#endif