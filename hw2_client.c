#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 10000
#define SERV_PORT 12260

static int sockfd;
static FILE *fp;
static int done;
static int getlock;

void * recvside(void *arg);

int main(int argc, char **argv)
{
	char sendbuf[BUFSIZE];
	pthread_t tid;
	struct sockaddr_in servaddr;
	ssize_t nbyte;

	sockfd=socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_port        = htons(SERV_PORT);
	inet_pton(AF_INET,"localhost",&servaddr.sin_addr);
	
	connect(sockfd,(const struct sockaddr *)&servaddr,sizeof(servaddr));
	
	fp=stdin;

	pthread_create(&tid,NULL,recvside,NULL);
	//send side
	printf("name:\n");
	fgets(sendbuf,BUFSIZE,fp);
	nbyte=send(sockfd,sendbuf,strlen(sendbuf),0);
	memset(sendbuf,0,sizeof(sendbuf));
	getlock=0;
	while(1)
	{
		if(getlock)
		{
			continue;		
		}
		if(fgets(sendbuf,BUFSIZE,fp)!=NULL)
		{
			nbyte=send(sockfd,sendbuf,strlen(sendbuf),0);
			if(strcmp(sendbuf,"exit\n")==0)
				break;
			memset(sendbuf,0,sizeof(sendbuf));
		}
		else
			break;
	}
	close(sockfd);
	done=1;
	return 0;
}

void * recvside(void *arg)
{
	//recv side
	ssize_t nbyte;
	char recvbuf[BUFSIZE];
	char new_recvbuf[BUFSIZE];
	int tempfd;
	int recvflag;
	recvflag=0;
	while(recv(sockfd,recvbuf,BUFSIZE,0)>0)
	{
		printf("%s",recvbuf);
		if(strcmp(recvbuf,"send to who?\n")==0)
		{
			memset(recvbuf,0,sizeof(recvbuf));
			getlock=1;
			nbyte=send(sockfd,recvbuf,strlen(recvbuf),0);//send name
			memset(recvbuf,0,sizeof(recvbuf));
			nbyte=send(sockfd,recvbuf,strlen(recvbuf),0);//send mes context
			printf("recv mes context: %s",recvbuf);
			memset(recvbuf,0,sizeof(recvbuf));
			getlock=0;
			
		}
		else if(strcmp(recvbuf,"recv file?(y/n)\n")==0)
		{
			printf("answer:\n");
			memset(recvbuf,0,sizeof(recvbuf));
			getlock=1;
			//fgets(recvbuf,BUFSIZE,stdin);
			//nbyte=send(sockfd,recvbuf,strlen(recvbuf),0);
			//memset(recvbuf,0,sizeof(recvbuf));
			getlock=0;
		}
		else if(strcmp(recvbuf,"accept\n")==0)
		{
			getlock=1;
			memset(recvbuf,0,sizeof(recvbuf));
			tempfd=open("./a.txt",O_RDONLY,0666);
			nbyte=read(tempfd,recvbuf,BUFSIZE);
			nbyte=send(sockfd,recvbuf,strlen(recvbuf),0);
			printf("file read done.\n");
			close(tempfd);
			getlock=0;
		}
		else if(strcmp(recvbuf,"recvfile")==0)
		{
			recvflag=1;
			tempfd=open("./b.txt",O_WRONLY|O_CREAT,0666);
		}
		else if(recvflag)
		{
			nbyte=write(tempfd,recvbuf,strlen(recvbuf));
			printf("file write done.\n");
			close(tempfd);
			recvflag=0;
		}
		memset(recvbuf,0,sizeof(recvbuf));
	}
	if(done==0)
	{
		printf("server terninated\n");
		exit(0);
	}

	return NULL;
}



