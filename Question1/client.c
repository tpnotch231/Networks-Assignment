#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include"messageDef.h"
#define RETRANSMISSIONRATE 2
//PACKET_SIZE is defined in header file messageDef.h, to facilitate similar definitions in client and server.
int main(int argc, char *argv[])
{
	if(argc==1)
	{
	    printf("No input file name was given. Assuming file is called input.txt and proceeding.\n");
	}
	int s,file;
	if((file=open(argc==1?"input.txt":argv[1]))==-1)
	{
	    perror("Error opening input file");
	    return 0;
	}
	Packet p;
	p.channelID=fork()==0?0:1;
	s=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	fd_set fdset;
	SETACKPACKET(p);
	SETNOTLASTPACKET(p);
	struct timeval timeout;
	struct sockaddr_in add;
	FD_ZERO(fdset);
	memset(&add,0,sizeof(add));
	add.sin_family=AF_INET;
	add.sin_port=htons(PORT_NO);
	add.sin_addr.s_addr=inet_addr("127.0.0.1");
	if(-1==connect(s,add,sizeof(struct sockaddr_in)))
	{
	    perror("Error creating a connection");
	    return 0;
	}
	FD_SET(s,fdset);
	while(!p.lastPacket)
	{
		p.seqNo=lseek(file,0,SEEK_CUR);
		p.payloadSize=read(file,p.payload,PACKET_SIZE);
		if(p.payloadSize<PACKET_SIZE)
			SETLASTPACKET(p);
		SETDATAPACKET(p);
		do{
			timeout.tv_sec=RETRANSMISSIONRATE;
			timeout.tv_usec=0;
			send(s,&p,sizeof(Packet),0);
			printf("SEND PKT: Seq. No %d of size %d Bytes from channel %d.\n",p.seqNo,p.payloadSize,p.channelID);
		}while(0==select(1,fdset,NULL,NULL,&timeout));
		recv(s,&p,sizeof(p),0);
		printf("RCVD ACK: for PKT with Seq. No %d from channel %d.\n",p.seqNo,p.channelID);
	}
	if(-1==close(s))
	{
	    perror("Error closing a socket");
	}
	if(-1==close(file))
	{
		perror("Error closing input file");
	}
	return 0;
}
