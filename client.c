#include<stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include<pthread.h>
#include<signal.h>

#define PORT 30000
#define BUFFER_SIZE 1024

int status = 1;

void *recv_handler(void *sock_fd)
{
	const int *temp = (int *)sock_fd;
	char buffer[BUFFER_SIZE] = {0};

	while(1)
	{
		int ret = recv(*temp, buffer, sizeof(buffer), 0);
		if(ret > 0)
		{
			buffer[ret] = '\0';
			printf("\t\t%s", buffer);	
		}
	}

	pthread_exit(0);
}

void *write_handler(void *sock_fd)
{
	const int *temp = (int *)sock_fd;
	char buffer[BUFFER_SIZE] = {0};

	while(1)
	{
		/* printf(">"); */
		fgets(buffer, sizeof(buffer), stdin);
		write(*temp, buffer, strlen(buffer));
	}

	pthread_exit(0);
}

void out(int sig)
{
	if(sig == SIGINT)
		status = 0;
}

int main()
{
	int sock_fd;
	struct sockaddr_in serv_addr;

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd == -1)
	{
		printf("Socket create failed\n");
		perror("Error");
		exit(0);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(PORT);

	if(0 != connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)))
	{
		printf("Socket connect failed\n");
		perror("Error");
		exit(0);
	}
	else
		printf("Socket successfully connected\n");

	signal(SIGINT, out);
	signal(SIGHUP, out);
	signal(SIGQUIT, out);

	pthread_t recv_thread, write_thread;
	pthread_create(&recv_thread, NULL, recv_handler, (void *)&sock_fd);
	pthread_create(&write_thread, NULL, write_handler, (void *)&sock_fd);
	
	/*
	pthread_join(recv_thread, NULL);
	pthread_join(write_thread, NULL);
	*/

	while(1)
	{
		if(!status)
		{
			write(sock_fd, "_out_", 5);
			close(sock_fd);
			break;
		}
	}

	return 0;
}