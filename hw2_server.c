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

#define	LISTENQ	1024
#define BUFSIZE 4000
#define NAMESIZE 20
#define SERV_PORT 12260
#define MAXCLI 10

typedef struct _clientdata{
	char name[NAMESIZE];
	int fd;
	char buf[BUFSIZE];
	int status;//avoid client exit 0=offline 1=online
} clientdata;

clientdata cliList[MAXCLI];
pthread_t tid[MAXCLI];
int alldown=0;
int clinum_max=0;

void* userfunc(void *arg);
void listall(int clinum);
void serverdown(int signo);
void clientdown(int clinum);
void mesall(int clinum);
void mesto(int clinum);
void fileto(int clinum);

int main(int argc, char **argv)
{
	int	listenfd,i, *iptr;
	socklen_t	clilen;
	struct sockaddr_in	cliaddr, servaddr;

	int yes=1;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);

	bind(listenfd, (const struct sockaddr *) &servaddr, sizeof(servaddr));

	listen(listenfd, LISTENQ);
	
	memset(cliList,0,sizeof(cliList));

	signal(SIGINT,serverdown);
	
	clinum_max=0;
	for(clinum_max=0;clinum_max<MAXCLI;clinum_max++)
	{
		cliList[clinum_max].fd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
		pthread_create(&tid[clinum_max],NULL,&userfunc,(void*)clinum_max);
	}
	serverdown(0);
	return 0;
}

void* userfunc(void *arg)
{
	ssize_t nbyte;
	int clinum=(int)arg;
	int readlock;
	memset(cliList[clinum].buf,0,sizeof(cliList[clinum].buf));
	nbyte = recv(cliList[clinum].fd,cliList[clinum].buf,BUFSIZE,0);
	strcpy(cliList[clinum].name,cliList[clinum].buf);
	cliList[clinum].name[strlen(cliList[clinum].name)-1]=0;//avoid \n
	cliList[clinum].status=1;
	printf("new client: id:%d %s\n",clinum,cliList[clinum].name);
	
	//server listen every client
	while(cliList[clinum].status)
	{
		memset(cliList[clinum].buf,0,sizeof(cliList[clinum].buf));
		printf("wait for recv...\n");
		nbyte = recv(cliList[clinum].fd,cliList[clinum].buf,BUFSIZE,0);
		cliList[clinum].buf[strlen(cliList[clinum].buf)-1]=0;
		if(nbyte)
			printf("%s\n",cliList[clinum].buf);
		if(strcmp(cliList[clinum].buf,"exit")==0)
		{
			printf("id:%d is down\n",clinum);
			strcpy(cliList[clinum].name,"");
			close(cliList[clinum].fd);
			memset(cliList[clinum].buf,0,sizeof(cliList[clinum].buf));
			cliList[clinum].status=0;
			break;
		}
		else if(strcmp(cliList[clinum].buf,"list")==0)
		{
			listall(clinum);
		}
		else if(strcmp(cliList[clinum].buf,"mesall")==0)
		{
			mesall(clinum);
		}
		else if(strcmp(cliList[clinum].buf,"mes")==0)
		{
			mesto(clinum);
		}
		else if(strcmp(cliList[clinum].buf,"file")==0)
		{
			fileto(clinum);
		}
		//printf("%d\n",strcmp(cliList[clinum].buf,"exit"));
		if(alldown)
		{
			break;
		}
	}
	//pthread_detach(pthread_self());
	return NULL;
}

void listall(int clinum)
{
	int i;
	for(i=0;i<clinum_max;i++)
	{
		if(cliList[clinum].status==0)
		{continue;}
		memset(cliList[clinum].buf,0,sizeof(cliList[clinum].buf));
		strcpy(cliList[clinum].buf,cliList[i].name);
		send(cliList[clinum].fd,cliList[clinum].buf,strlen(cliList[clinum].buf),0);
		if(i!=clinum_max-1)
			send(cliList[clinum].fd,",",strlen(","),0);
	}
	send(cliList[clinum].fd,"\n",strlen("\n"),0);
}

void serverdown(int signo)
{
	int i;
	printf("server down\n");
	for(i=0;i<clinum_max;i++)
	{
		if(clinum_max)//avoid no client
		{
			clientdown(i);
			pthread_detach(tid[i]);
		}
	}
	memset(cliList,0,sizeof(cliList));
	pthread_exit(NULL);
	return;
}

void clientdown(int clinum)
{
	printf("id:%d is down\n",clinum);
	strcpy(cliList[clinum].name,"");
	close(cliList[clinum].fd);
	memset(cliList[clinum].buf,0,sizeof(cliList[clinum].buf));
	cliList[clinum].status=0;
	return;
}

void mesall(int clinum)
{
	ssize_t nbyte;
	int i;
	memset(cliList[clinum].buf,0,sizeof(cliList[clinum].buf));
	strcpy(cliList[clinum].buf,"input message to all:\n");
	nbyte = send(cliList[clinum].fd,cliList[clinum].buf,BUFSIZE,0);//send input mes
	memset(cliList[clinum].buf,0,sizeof(cliList[clinum].buf));
	nbyte = recv(cliList[clinum].fd,cliList[clinum].buf,BUFSIZE,0);//get mes
	for(i=0;i<clinum_max;i++)
	{
		if(cliList[i].status)
		{
			send(cliList[i].fd,cliList[clinum].buf,BUFSIZE,0);
		}
	}
	return;
}

void mesto(int clinum)
{
	ssize_t nbyte;
	int i,target;
	memset(cliList[clinum].buf,0,sizeof(cliList[clinum].buf));
	strcpy(cliList[clinum].buf,"send to who?\n");
	nbyte = send(cliList[clinum].fd,cliList[clinum].buf,BUFSIZE,0);//send input mes
	memset(cliList[clinum].buf,0,sizeof(cliList[clinum].buf));
	nbyte = recv(cliList[clinum].fd,cliList[clinum].buf,BUFSIZE,0);//get name
	cliList[clinum].buf[strlen(cliList[clinum].buf)-1]=0;//avoid \n
	printf("recv name: %s\n",cliList[clinum].buf);

	target= -1;
	for(i=0;i<clinum_max;i++)
	{
		if(strcmp(cliList[clinum].buf,cliList[i].name)==0)
		{
			target=i;
			break;
		}				
	}
	if(i==clinum_max&&target== -1 )//search fail
	{
		memset(cliList[clinum].buf,0,sizeof(cliList[clinum].buf));
		strcpy(cliList[clinum].buf,"can't find this user\n");
		send(cliList[clinum].fd,cliList[clinum].buf,BUFSIZE,0);
	}
	else
	{
		memset(cliList[clinum].buf,0,sizeof(cliList[clinum].buf));
		nbyte = recv(cliList[clinum].fd,cliList[clinum].buf,BUFSIZE,0);//get mes context
		printf("recv context: %s",cliList[clinum].buf);
		if(cliList[target].status)
		{
			send(cliList[target].fd,cliList[clinum].buf,BUFSIZE,0);//send mes to target
		}
	}
	return;
}

void fileto(int clinum)
{
	ssize_t nbyte;
	int i,target;
	memset(cliList[clinum].buf,0,sizeof(cliList[clinum].buf));
	strcpy(cliList[clinum].buf,"send to who?\n");
	nbyte = send(cliList[clinum].fd,cliList[clinum].buf,BUFSIZE,0);//send input mes
	memset(cliList[clinum].buf,0,sizeof(cliList[clinum].buf));
	nbyte = recv(cliList[clinum].fd,cliList[clinum].buf,BUFSIZE,0);//get name
	cliList[clinum].buf[strlen(cliList[clinum].buf)-1]=0;//avoid \n
	target= -1;
	for(i=0;i<clinum_max;i++)
	{
		if(strcmp(cliList[clinum].buf,cliList[i].name)==0)
		{
			target=i;
			break;
		}				
	}
	if(i==clinum_max&&target== -1 )//search fail
	{
		memset(cliList[clinum].buf,0,sizeof(cliList[clinum].buf));
		strcpy(cliList[clinum].buf,"can't find this user\n");
		send(cliList[clinum].fd,cliList[clinum].buf,BUFSIZE,0);
	}
	else
	{
		nbyte = recv(cliList[clinum].fd,cliList[clinum].buf,BUFSIZE,0);//get context(dump)
		if(cliList[target].status)
		{
			memset(cliList[target].buf,0,sizeof(cliList[target].buf));
			strcpy(cliList[target].buf,"recv file?(y/n)\n");
			nbyte = send(cliList[target].fd,cliList[target].buf,BUFSIZE,0);//send mes to receiver
			memset(cliList[target].buf,0,sizeof(cliList[target].buf));
			nbyte = recv(cliList[target].fd,cliList[target].buf,BUFSIZE,0);//get y/n
			if(strcmp(cliList[target].buf,"y\n")==0)
			{	
				strcpy(cliList[clinum].buf,"accept\n");
				nbyte = send(cliList[clinum].fd,cliList[clinum].buf,BUFSIZE,0);//send request to sender	
				strcpy(cliList[target].buf,"recvfile");
				nbyte = send(cliList[target].fd,cliList[target].buf,sizeof(cliList[target].buf),0);//send mes to receiver
				memset(cliList[target].buf,0,sizeof(cliList[target].buf));	
				//printf("%s\n",cliList[clinum].buf);
				nbyte = recv(cliList[clinum].fd,cliList[clinum].buf,sizeof(cliList[clinum].buf),0);//get file from sender
				nbyte = send(cliList[target].fd,cliList[clinum].buf,sizeof(cliList[clinum].buf),0);//send file to receiver
			}
		}
	}
	return;
}
