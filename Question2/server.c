#define _POSIX_C_SOURCE 200809L
#include<stdio.h>
#include<unistd.h>
#include<time.h>
#include<string.h>
#include<stdbool.h>
#include<fcntl.h>
#include<limits.h>
#include<sys/time.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include"packet.h"
void copyRange(char *dest, int db, int de, char *src, int sb, int se)
{
    for(int i=db,j=sb;i<de&&j<se;i++,j++)
    {
        dest[i]=src[j];
    }
    return;
}
bool readIntoBuffer(char *buffer,int *writeSize, fd_set fdset,int r1,int r2,struct sockaddr_in *r1Addr, struct sockaddr_in *r2Addr,int *r1len, int *r2len)
{
    static int beg=0, end=0, lastPayload=INT_MAX,lastPayloadS;
    static bool arr[WINDOWSIZE]={false},endFin=false;
    struct sockaddr_in temp;
    Packet p;
    struct timespec ts;
    while(end-beg<WINDOWSIZE/2&&lastPayload>=end)
    {
        select((r1>r2?r1:r2)+1,&fdset,NULL,NULL,NULL);
        if(FD_ISSET(r1,&fdset))
        {
            clock_gettime(CLOCK_REALTIME,&ts);
            if(recvfrom(r1,&p,sizeof p,0,(struct sockaddr*)r1Addr,r1len)==-1)
            {
                perror("Error receiving the message");
                continue;
                //It will just be treated as a drop
            }
            printf("SERVER");
            printf("\tR");
            printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
            printf(p.dataPacket?"DATA":"ACK");
            printf("\t%d",p.seqNo);
            printf("\tRELAY1");
            printf("\tSERVER\n");
            if(p.seqNo>=beg+WINDOWSIZE) continue;
            SETACKPACKET(p);
            clock_gettime(CLOCK_REALTIME,&ts);
            if(sendto(r1,&p,sizeof p,0,(struct sockaddr*)r1Addr,sizeof(struct sockaddr))==-1)
            {
                perror("Error transmitting packet");
                continue;
            }
            printf("SERVER");
            printf("\tS");
            printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
            printf(p.dataPacket?"DATA":"ACK");
            printf("\t%d",p.seqNo);
            printf("\tSERVER");
            printf("\tRELAY1\n");
            if(p.seqNo<beg) continue;
            if(p.seqNo>=end)
            {
                //write(STDOUT_FILENO,p.payload,p.payloadSize);
                //putchar('\n');
                copyRange(buffer,(p.seqNo-beg)*PACKET_SIZE,(p.seqNo-beg+1)*PACKET_SIZE,p.payload,0,p.payloadSize);
                arr[p.seqNo-beg]=true;
                if(p.lastPacket)
                {
                    lastPayload=p.seqNo;
                    lastPayloadS=p.payloadSize;
                }
            }
            if(p.seqNo==end)
            {
                while(end-beg<WINDOWSIZE&&arr[end-beg]) end++;
            }
        }
        if(FD_ISSET(r2,&fdset))
        {
            clock_gettime(CLOCK_REALTIME,&ts);
            if(recvfrom(r2,&p,sizeof p,0,(struct sockaddr*)r2Addr,r2len)==-1)
            {
                perror("Error receiving the message");
                continue;
                //It will just be treated as a drop
            }
            printf("SERVER");
            printf("\tR");
            printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
            printf(p.dataPacket?"DATA":"ACK");
            printf("\t%d",p.seqNo);
            printf("\tRELAY2");
            printf("\tSERVER\n");
            if(p.seqNo>=beg+WINDOWSIZE) continue;
            SETACKPACKET(p);
            clock_gettime(CLOCK_REALTIME,&ts);
            if(sendto(r2,&p,sizeof p,0,(struct sockaddr*)r2Addr,sizeof(struct sockaddr))==-1)
            {
                perror("Error transmitting packet");
                continue;
            }
            printf("SERVER");
            printf("\tS");
            printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
            printf(p.dataPacket?"DATA":"ACK");
            printf("\t%d",p.seqNo);
            printf("\tSERVER");
            printf("\tRELAY2\n");
            if(p.seqNo<beg) continue;
            if(p.seqNo>=end)
            {
                //write(STDOUT_FILENO,p.payload,p.payloadSize);
                //putchar('\n');
                copyRange(buffer,(p.seqNo-beg)*PACKET_SIZE,(p.seqNo-beg+1)*PACKET_SIZE,p.payload,0,p.payloadSize);
                arr[p.seqNo-beg]=true;
                if(p.lastPacket)
                {
                    lastPayload=p.seqNo;
                    lastPayloadS=p.payloadSize;
                }
            }
            if(p.seqNo==end)
            {
                while(end-beg<WINDOWSIZE&&arr[end-beg]) end++;
            }
        }
        FD_SET(r1,&fdset);
        FD_SET(r2,&fdset);
    }
    *writeSize=(end-beg)*PACKET_SIZE;
    if(lastPayload<end)
    {
        *writeSize-=PACKET_SIZE;
        *writeSize+=lastPayloadS;
        endFin=true;
    }
    for(int i=end;i-beg<WINDOWSIZE;i++)
    {
        arr[i-end]=arr[i-beg];
    }
    for(int i=WINDOWSIZE-1,j=end;j>beg;i--,j--)
    {
        arr[i]=false;
    }
    beg=end;
    return endFin;
}
int main(int argc, char *argv[])
{
    char buffer[WINDOWSIZE*PACKET_SIZE];
    int r1, r2, r1len, r2len, file, writeSize;
    fd_set fdset;
    struct sockaddr_in r1Addr, r2Addr;
    if(argc==1)
    {
        printf("No output file name given. Assuming file name is output.txt and proceeding.\n");
    }
    if((file=open(argc==1?"output.txt":argv[1],O_CREAT|O_WRONLY))==-1)
    {
        perror("Error opening the output file");
        return 0;
    }
    memset(&r1Addr,0,sizeof r1Addr);
    memset(&r2Addr,0,sizeof r2Addr);
    r2Addr.sin_family=r1Addr.sin_family=AF_INET;
    r1Addr.sin_port=htons(SERVER_PORT1);
    r2Addr.sin_port=htons(SERVER_PORT2);
    r2Addr.sin_addr.s_addr=r1Addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    r2len=r1len=sizeof (struct sockaddr);
    if((r1=socket(PF_INET,SOCK_DGRAM,0))==-1)
    {
        perror("Error creating the first socket");
        close(file);
        return 0;
    }
    if((r2=socket(PF_INET,SOCK_DGRAM,0))==-1)
    {
        perror("Error creating the second socket");
        close(file);
        close(r1);
        return 0;
    }
    if(bind(r1,(struct sockaddr*)&r1Addr,sizeof r1Addr)==-1)
    {
        perror("Error binding the first socket");
        close(file);
        close(r1);
        close(r2);
        return 0;
    }
    if(bind(r2,(struct sockaddr*)&r2Addr,sizeof r2Addr)==-1)
    {
        perror("Error binding the second socket");
        close(file);
        close(r1);
        close(r2);
        return 0;
    }
    FD_ZERO(&fdset);
    FD_SET(r1,&fdset);
    FD_SET(r2,&fdset);
    while(1)
    {
        bool temp=readIntoBuffer(buffer,&writeSize,fdset,r1,r2,&r1Addr,&r2Addr,&r1len,&r2len);
        if(write(file,buffer,writeSize)==-1)
        {
            perror("Error writing to output file");
        }
        copyRange(buffer,0,PACKET_SIZE*WINDOWSIZE,buffer,writeSize,WINDOWSIZE*PACKET_SIZE);
        if(temp) break;
    }
    close(file);
    close(r1);
    close(r2);
    return 0;
}