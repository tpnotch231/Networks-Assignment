#define _POSIX_C_SOURCE 200809L
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<time.h>
#include<stdlib.h>
#include<inttypes.h>
#include<sys/time.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include"packet.h"
#define PDR 10
void delay(int us)
{
	struct timespec temp={0,1000*us};
	nanosleep(&temp,NULL);
	return;
}
int main(int argc, char *argv[])
{
    int rsock, tsock, rsize, clen, slen, rn;
    fd_set fdset;
    struct sockaddr_in cliAddr, serAddr;
    Packet p;
    struct timespec ts;
    if(argc==1)
    {
        printf("Relay node unspecified. Enter 1 or 2 to specify.\n");
        return 0;
    }
    rn=strcmp(argv[1],"1")?2:1;
    SETNOTLASTPACKET(p);
    FD_ZERO(&fdset);
    srand(time(0));
    memset(&cliAddr,0,sizeof cliAddr);
    memset(&serAddr,0,sizeof serAddr);
    cliAddr.sin_family=AF_INET;
    cliAddr.sin_port=htons(rn==1?RELAY_PORT1:RELAY_PORT2);
    cliAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    serAddr.sin_family=AF_INET;
    serAddr.sin_port=htons(rn==1?SERVER_PORT1:SERVER_PORT2);
    serAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    if((rsock=socket(PF_INET,SOCK_DGRAM,0))==-1)
    {
        perror("Error creating the receiving socket");
        return 0;
    }
    if((tsock=socket(PF_INET,SOCK_DGRAM,0))==-1)
    {
        perror("Error creating the transmitting socket");
        return 0;
    }
    if(bind(rsock,(struct sockaddr*)&cliAddr,sizeof cliAddr)==-1)
    {
        perror("Error binding the receiving socket to the port");
        return 0;
    }
    FD_SET(rsock,&fdset);
    FD_SET(tsock,&fdset);
    printf("Current time in timestamp measured as seconds:nanoseconds.\n");
    while(select((rsock>tsock?rsock:tsock)+1,&fdset,NULL,NULL,NULL))
    {
        if(FD_ISSET(rsock,&fdset))
        {
            clock_gettime(CLOCK_REALTIME,&ts);
            if((rsize=recvfrom(rsock,&p,sizeof p,0,(struct sockaddr*)&cliAddr,&clen))==-1)
            {
                perror("Error receiving packet from client");
                //Not returning here as even if there's an error the result will be similar to a dropped packet.
                continue;
            }
            printf(rn==1?"RELAY1":"RELAY2");
            printf("\tR");
            printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
            printf(p.dataPacket?"DATA":"ACK");
            printf("\t%d",p.seqNo);
            printf("\tCLIENT");
            printf(rn==1?"\tRELAY1\n":"\tRELAY2\n");
            if(rand()%100<PDR)
            {
                clock_gettime(CLOCK_REALTIME,&ts);
                SETNOTLASTPACKET(p);
                printf(rn==1?"RELAY1":"RELAY2");
                printf("\tD");
                printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
                printf(p.dataPacket?"DATA":"ACK");
                printf("\t%d",p.seqNo);
                printf("\tCLIENT");
                printf(rn==1?"\tRELAY1\n":"\tRELAY2\n");
                continue;
            }
            delay(rand()%2001);
            clock_gettime(CLOCK_REALTIME,&ts);
            if(sendto(tsock,&p,sizeof p,0,(struct sockaddr*)&serAddr,sizeof serAddr)==-1)
            {
                perror("Error transmitting the message to the server");
                //Not returning here as even if there's an error the result will be similar to a droped packet.
            }
            printf(rn==1?"RELAY1":"RELAY2");
            printf("\tS");
            printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
            printf(p.dataPacket?"DATA":"ACK");
            printf("\t%d",p.seqNo);
            printf(rn==1?"\tRELAY1":"\tRELAY2");
            printf("\tSERVER\n");
        }
        if(FD_ISSET(tsock,&fdset))
        {
            clock_gettime(CLOCK_REALTIME,&ts);
            if((rsize=recvfrom(tsock,&p,sizeof p,0,(struct sockaddr*)&serAddr,&slen))==-1)
            {
                perror("Error receiving ack from server");
                continue;
                //Not returning here as even if there's an error the result will be similar to a droped packet.
            }
            printf(rn==1?"RELAY1":"RELAY2");
            printf("\tR");
            printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
            printf(p.dataPacket?"DATA":"ACK");
            printf("\t%d",p.seqNo);
            printf("\tSERVER");
            printf(rn==1?"\tRELAY1\n":"\tRELAY2\n");
            clock_gettime(CLOCK_REALTIME,&ts);
            if(sendto(rsock,&p,sizeof p,0,(struct sockaddr*)&cliAddr,sizeof cliAddr)==-1)
            {
                perror("Error transmitting the ack to the client");
                //Not returning here as even if there's an error the result will be similar to a droped packet.
            }
            printf(rn==1?"RELAY1":"RELAY2");
            printf("\tS");
            printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
            printf(p.dataPacket?"DATA":"ACK");
            printf("\t%d",p.seqNo);
            printf(rn==1?"\tRELAY1":"\tRELAY2");
            printf("\tCLIENT\n");
        }
        FD_SET(rsock,&fdset);
        FD_SET(tsock,&fdset);
    }
}