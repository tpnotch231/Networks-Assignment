#define _POSIX_C_SOURCE 200809L
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdbool.h>
#include<stdlib.h>
#include<time.h>
#include<fcntl.h>
#include<math.h>
#include<signal.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include"packet.h"
#define RETRANSMISSION_RATE 3       //milliseconds
typedef struct queue{
    struct timespec ts;
    int ind;
    struct queue *next, *prev;
} QueueNode;
typedef QueueNode* Queue[2];
typedef struct packetlist{
    Packet p;
    int ind;
    bool acked;
    struct packetlist *next;
} PacketList;

PacketList* packetAtInd(int ind, PacketList *p)
{
    PacketList *temp=p;
    while(temp!=NULL&&temp->ind!=ind)
    {
        temp=temp->next;
    }
    return temp;
}

void insertPacket(Packet p,int ind, PacketList *head)
{
    PacketList *temp=malloc(sizeof(PacketList));
    temp->next=NULL;
    temp->p=p;
    temp->ind=ind;
    temp->acked=false;
    while(head->next!=NULL)
    {
        head=head->next;
    }
    head->next=temp;
}
PacketList* removeHead(PacketList *head)
{
    PacketList *temp=head;
    head=head->next;
    free(temp);
    return head;
}
void enqueue(Queue q,int ind)
{
    QueueNode *n=malloc(sizeof(QueueNode));
    n->next=NULL;
    n->prev=NULL;
    n->ind=ind;
    clock_gettime(CLOCK_REALTIME,&(n->ts));
    //n->ts.tv_nsec+RETRANSMISSION_RATE*1000000;
    n->ts.tv_sec+=RETRANSMISSION_RATE;
    if(n->ts.tv_nsec>=1000000000)
    {
        n->ts.tv_sec+=n->ts.tv_nsec/1000000000;
        n->ts.tv_nsec%=1000000000;
    }
    if(q[0]!=NULL)
    {
        q[1]->next=n;
        n->prev=q[1];
        q[1]=n;
    }
    else
    {
        q[0]=n;
        q[1]=n;
    }
}
QueueNode* dequeue(Queue q)
{
    QueueNode *temp=q[0];
    if(temp!=NULL)
    {
        q[0]=q[0]->next;
        temp->next=NULL;
        q[0]->prev=NULL;
    }
    return temp;
}
int main(int argc, char *argv[])
{
    int file,count,r1,r2,endFile,r1len,r2len;
    fd_set fdset;
    struct sockaddr_in r1Addr, r2Addr;
    Queue tqueue={NULL,NULL};
    Packet p1;
    PacketList *p=malloc(sizeof(PacketList));
    struct timespec ts;
    p->next=NULL;
    if(argc==1)
    {
        printf("No input file name specified. Assuming file is named input.txt and proceeding.\n");
    }
    if((file=open(argc==1?"input.txt":argv[1],O_RDONLY))==-1)
    {
        perror("Error opening input file");
        return 0;
    }
    if((r1=socket(PF_INET,SOCK_DGRAM,0))==-1)
    {
        perror("Error creating the first relay socket");
        close(file);
        return 0;
    }
    if((r2=socket(PF_INET,SOCK_DGRAM,0))==-1)
    {
        perror("Error opening second relay socket");
        close(file);
        close(r1);
        return 0;
    }
    signal(SIGINT,SIG_IGN);
    memset(&r1Addr,0,sizeof r1Addr);
    memset(&r2Addr,0,sizeof r2Addr);
    r2Addr.sin_family=r1Addr.sin_family=AF_INET;
    r1Addr.sin_port=htons(RELAY_PORT1);
    r2Addr.sin_port=htons(RELAY_PORT2);
    r2Addr.sin_addr.s_addr=r1Addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    FD_ZERO(&fdset);
    count=ceil(lseek(file,0,SEEK_END)*1.0/PACKET_SIZE);
    endFile=lseek(file,0,SEEK_END);
    lseek(file,0,SEEK_SET);
    tqueue[0]=tqueue[1]=NULL;
    
    if((p->p.payloadSize=read(file,&p->p.payload,PACKET_SIZE))==-1)
    {
        perror("Error reading from file");
        close(file);
        close(r1);
        close(r2);
        return 0;
    }
    p->p.seqNo=0;
    if(lseek(file,0,SEEK_CUR)==endFile)
    {
        SETLASTPACKET(p->p);
    }
    else
    {
        SETNOTLASTPACKET(p->p);
    }
    SETDATAPACKET(p->p);
    clock_gettime(CLOCK_REALTIME,&ts);
    if(sendto(r2,&p->p,sizeof p->p,0,(struct sockaddr*)&r2Addr,sizeof r2Addr)==-1)
    {
        perror("Error sending packet");
        close(file);
        close(r1);
        close(r2);
        return 0;
    }
    printf("CLIENT\tS");
    printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
    printf("DATA\t0\tCLIENT\tRELAY2\n");

    enqueue(tqueue,0);
    int i;
    for(i=1;i<(WINDOWSIZE<count?WINDOWSIZE:count);i++)
    {
        p1.seqNo=i;
        p1.payloadSize=read(file,&p1.payload,PACKET_SIZE);
        if(p1.payloadSize==-1)
        {
            perror("Error reading from file");
        }
        SETNOTLASTPACKET(p1);
        if(lseek(file,0,SEEK_CUR)==endFile) SETLASTPACKET(p1);
        SETDATAPACKET(p1);
        insertPacket(p1,i,p);
        clock_gettime(CLOCK_REALTIME,&ts);
        if(sendto(i%2?r1:r2,&p1,sizeof p1,0,(struct sockaddr*)(i%2?&r1Addr:&r2Addr),i%2?sizeof r1Addr:sizeof r2Addr)==-1)
        {
            perror("Error sending packet");
            close(file);
            close(r1);
            close(r2);
            return 0;
        }
        printf("CLIENT");
        printf("\tS");
        printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
        printf("DATA");
        printf("\t%d",p1.seqNo);
        printf("\tCLIENT");
        printf(i%2?"\tRELAY1\n":"\tRELAY2\n");
        enqueue(tqueue,i);
    }
    do{
        struct timespec temp;
        int retur;
        clock_gettime(CLOCK_REALTIME,&temp);
        temp.tv_sec=tqueue[0]->ts.tv_sec-temp.tv_sec;
        temp.tv_nsec=tqueue[0]->ts.tv_nsec-temp.tv_nsec;
        if(temp.tv_nsec<0)
        {
            temp.tv_nsec+=1000000000;
            temp.tv_sec--;
        }
        if(temp.tv_sec<0)
        {
            temp.tv_sec=0;
            temp.tv_nsec=0;
        }
        FD_SET(r1,&fdset);
        FD_SET(r2,&fdset);
        if(0==(retur=pselect((r1>r2?r1:r2)+1,&fdset,NULL,NULL,&temp,NULL)))
        {
            int i1=dequeue(tqueue)->ind;
            if(i1<p->ind) continue;
            clock_gettime(CLOCK_REALTIME,&ts);
            printf("CLIENT");
            printf("\tTO");
            printf("\t%ld:%ld",ts.tv_sec,ts.tv_nsec);
            printf("\t\t%d\n",i1);
            p1=packetAtInd(i1,p)->p;
            clock_gettime(CLOCK_REALTIME,&ts);
            if(sendto(i1%2?r1:r2,&p1,sizeof p1,0,(struct sockaddr*)(i1%2?&r1Addr:&r2Addr),i1%2?sizeof r1Addr:sizeof r2Addr)==-1)
            {
                perror("Error sending packet");
                close(file);
                close(r1);
                close(r2);
                return 0;
            }
            enqueue(tqueue,i1);
            printf("CLIENT");
            printf("\tRE");
            printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
            printf("DATA");
            printf("\t%d",p1.seqNo);
            printf("\tCLIENT");
            printf(i1%2?"\tRELAY1\n":"\tRELAY2\n");
        }
        else if(retur>0)
        {
            if(FD_ISSET(r1,&fdset))
            {
                clock_gettime(CLOCK_REALTIME,&ts);
                r1len=sizeof r1Addr;
                if(recvfrom(r1,&p1,sizeof p1,0,(struct sockaddr*)&r1Addr,&r1len)==-1)
                {
                    perror("Error receiving packet");
                    close(file);
                    close(r1);
                    close(r2);
                    return 0;
                }
                printf("CLIENT");
                printf("\tR");
                printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
                printf(p1.dataPacket?"DATA":"ACK");
                printf("\t%d",p1.seqNo);
                printf("\tRELAY1");
                printf("\tCLIENT\n");
                if(!p1.dataPacket&&p1.seqNo>=p->ind)
                packetAtInd(p1.seqNo,p)->acked=true;
            }
            else if(FD_ISSET(r2,&fdset))
            {
                clock_gettime(CLOCK_REALTIME,&ts);
                r2len=sizeof r2Addr;
                if(recvfrom(r2,&p1,sizeof p1,0,(struct sockaddr*)&r2Addr,&r2len)==-1)
                {
                    perror("Error receiving packet");
                    close(file);
                    close(r1);
                    close(r2);
                    return 0;
                }
                printf("CLIENT");
                printf("\tR");
                printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
                printf(p1.dataPacket?"DATA":"ACK");
                printf("\t%d",p1.seqNo);
                printf("\tRELAY2");
                printf("\tCLIENT\n");
                if(!p1.dataPacket&&p1.seqNo>=p->ind)
                packetAtInd(p1.seqNo,p)->acked=true;
            }
        }
        else
        {
            perror("Error with select");
        }
        while(p!=NULL&&p->acked==true)
        {
            p=removeHead(p);
            if(i<count)
            {
                p1.seqNo=i;
                p1.payloadSize=read(file,&p1.payload,PACKET_SIZE);
                if(p1.payloadSize==-1)
                {
                    perror("Error reading from file");
                }
                SETNOTLASTPACKET(p1);
                if(lseek(file,0,SEEK_CUR)==endFile) SETLASTPACKET(p1);
                SETDATAPACKET(p1);
                insertPacket(p1,i,p);
                clock_gettime(CLOCK_REALTIME,&ts);
                if(sendto(i%2?r1:r2,&p1,sizeof p1,0,(struct sockaddr*)(i%2?&r1Addr:&r2Addr),i%2?sizeof r1Addr:sizeof r2Addr)==-1)
                {
                    perror("Error sending packet");
                    close(file);
                    close(r1);
                    close(r2);
                    return 0;
                }
                enqueue(tqueue,i);
                printf("CLIENT");
                printf("\tS");
                printf("\t%ld:%ld\t",ts.tv_sec,ts.tv_nsec);
                printf("DATA");
                printf("\t%d",p1.seqNo);
                printf("\tCLIENT");
                printf(i%2?"\tRELAY1\n":"\tRELAY2\n");
                i++;
            }
        }
    }while(i<count||p!=NULL);
}