#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<time.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include"messageDef.h"
#define PDR 10			//Implemented as a percentage so must always be less than 100. Any value greater than 100 will make the program drop all the programs.
int file,listener,cs1;
void handler(int sig)
{
	if(sig==SIGUSR1)
		close(cs1);
	close(file);
	close(listener);
	exit(0);
}
int main(int argc, char *argv[])
{
	int child,count=0,cl1l,cl2l;
	struct sockaddr_in selfAddr, cl1Addr, cl2Addr;
	srand(time(0));
	signal(SIGUSR1,handler);
	//Purpose explained later.
	signal(SIGINT,handler);
	//Graceful termination for the parent process. Main way of stopping server. 
	if(argc==1)
	{
		printf("No output file name given. Assuming output file name is output.txt and proceeding.\n");
	}
	if((file=open(argc==1?"output.txt":argv[1],O_CREAT|O_WRONLY))==-1)
	{
		perror("Error opening file");
		return 0;
	}
	if((listener=socket(PF_INET,SOCK_STREAM,0))==-1)
	{
		perror("Error creating listening socket");
		return 0;
	}
	memset(&selfAddr,0,sizeof(selfAddr));
	selfAddr.sin_family=AF_INET;
	selfAddr.sin_port=htons(PORT_NO);
	selfAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	if((bind(listener,&selfAddr,sizeof(struct sockaddr_in)))==-1)
	{
		perror("Error binding listening socket");
		if(-1==close(listener))
		{
			perror("Error closing listening socket");
		}
		if(-1==close(file))
		{
			perror("Error closing file");
		}
		return 0;
	}
	if(-1==listen(listener,2))
	{
		perror("Error calling listen");
		if(-1==close(listener))
		{
			perror("Error closing listening socket");
		}
		if(-1==close(file))
		{
			perror("Error closing file");
		}
		return 0;
	}
	cl1l=sizeof cl1Addr;
	cl2l=sizeof cl2Addr;
	while(1)
	{
		cs1=accept(listener,&cl1Addr,&cl1l)
		if(cl1l<0)
		{
			perror("Error in accepting first socket");
			if(-1==close(listener))
			{
				perror("Error closing listening socket");
			}
			if(-1==close(file))
			{
				perror("Error closing file");
			}
			return 0;
		}
		else if((child=fork())==0)
		{
			break;
		}
		if(-1==close(cs1))
		{
			perror("Error closing channel in parent.");
		}
		cs1=accept(listener,&cl2Addr,&cl2l);
		if(cl2l<0)
		{
			perror("Error in accepting second socket");
			kill(child,SIGUSR1);
			if(-1==close(listener))
			{
				perror("Error closing listening socket");
			}
			if(-1==close(file))
			{
				perror("Error closing file");
			}
			//In case there is an error in opening the second connection the first connection must be closed.
			shutdown(cs1,2);
			return 0;
		}
		else if(cl1Addr.sin_addr!=cl2Addr.sin_addr)
		{
			//If two different programs try to contact the server, everything is stopped.
			kill(child,SIGUSR1);
			printf("Competing connections discovered.\n");
			if(-1==close(listener))
			{
				perror("Error closing listening port");
			}
			if(-1==close(file))
			{
				perror("Error closing file");
			}
			if(-1==close(cs1))
			{
				perror("Error closing second channel");
			}
			return 0;
		}
		else if(fork()==0)
		{
			break;
		}
	}
	if(-1==close(listener))
	{
		//This point is reached only by the forked process. They do not need the listening socket anymore.
		perror("Error closing the listening socket in child process");
		if(-1==close(cs1))
		{
			perror("Error closing the socket");
		}
		if(-1==close(file))
		{
			perror("Error closing the output file");
		}
		return 0;
	}
	Packet p;
	SETNOTLASTPACKET(p);
	while(!p.lastPacket)
	{
		recv(cs1,&p,sizeof(p),0);
		if(rand()/(RAND_MAX/100 + 1)<PDR)
		{
			SETNOTLASTPACKET(p);	//In case the packet dropped is the last packet.
			continue;
		}
		printf("RCVD PKT: Seq. No %d of size %d Bytes from channel %d.\n",p.seqNo,p.payloadSize,p.channelID);
		lseek(file,p.seqNo,SEEK_BEG);
		if(p.payloadSize> -1)			//To handle the last packet when file size is a multiple of packet size()
			write(file,p.payload,p.payloadSize);
		SETACKPACKET(p);
		send(cs1,&p,sizeof(p),0);
		printf("SENT ACK: for PKT with Seq. No %d from channel %d.\n",p.seqNo,p.channelID);
	}
	if(-1==close(cs1))
	{
		perror("Error closing the socket");
	}
	if(-1==close(file))
	{
		perror("Error closing the output file");
	}
	return 0;
}
