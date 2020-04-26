#ifndef MESSAGEDEF_H
#define MESSAGEDEF_H

#define PACKET_SIZE 100				//the size of the payload in this implementation, adjust accordingly
#define PORT_NO 8153
typedef struct{
	unsigned int payloadSize;
	unsigned int seqNo;
	unsigned int lastPacket:1;		//0 if not last packet, one if it is
	unsigned int dataPacket:1;		//0 if ack, 1 if data
	unsigned int channelID:1;		//The channel (0 or 1) through which this packet was/is to be sent.
	char payload[PACKET_SIZE];
}Packet;

#define SETDATAPACKET(p) p.dataPacket=1
#define SETACKPACKET(p) p.dataPacket=0
#define SETLASTPACKET(p) p.lastPacket=1
#define SETNOTLASTPACKET(p) p.lastPacket=0
#endif
