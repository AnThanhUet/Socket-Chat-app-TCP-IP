#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<sys/types.h>

#define PORT 30000

#define RECV_BUFFER_SIZE 1024 /* max size of TCP packages ~ 64Kb */
#define RECV_SIZE 1024 /* max size of ethernet packages 1500 bytes */
#define SEND_BUFFER_SIZE 1024 

#define MAX_CLIENTS 100

typedef struct{
    struct sockaddr_in addr;
    int sock_fd;
    char name[100];
} client_t;  

client_t *clients[MAX_CLIENTS];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void add_client(client_t *cli)
{
	pthread_mutex_lock(&mutex);
	int i;
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(NULL ==  clients[i])
		{
			clients[i] = cli;
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
}

void remove_client(int sock_fd)
{
	pthread_mutex_lock(&mutex);
	int i;
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(clients[i] != NULL && clients[i]->sock_fd == sock_fd)
		{
			clients[i] = NULL;
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
}

void set_name(client_t *cli, const char *name)
{
	name += 7;
	char *pos = strrchr(name, '\n');
	if(pos != NULL)
		*pos = '\0';
	strcpy(cli->name, name);
}

void receive(int sock_fd, char *recv_buffer)
{
	ssize_t ret;
	memset(recv_buffer, 0, RECV_BUFFER_SIZE);
	while(1)
	{
		ret = recv(sock_fd, recv_buffer, RECV_SIZE, 0);
		if(ret < 0)
		{
			printf("recv() error\n");
			break;
		}
		else
		{
			/* printf("%s", recv_buffer); */
			if(ret < RECV_SIZE)
				break;
			recv_buffer += ret;
		}
	}
}

void broadcast(char *mess, client_t *cli)
{
    int i;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        if(NULL != clients[i] && clients[i]->sock_fd != cli->sock_fd)
        {
			//sprintf(mess, "%d: %s", clients[i]->sock_fd, mess);
			char buffer[RECV_SIZE];
			sprintf(buffer, "[%s]: %s", cli->name, mess);
            write(clients[i]->sock_fd, buffer, strlen(buffer));
        }
    }
}

int check_key(char *buffer, char *key)
{
	unsigned int i;
	for(i = 0; i < strlen(key); i++)
	{
		if(buffer[i] != key[i])
			return 0;
	}
	return 1;
}

void *handler(void *arg)
{
    client_t *cli = (client_t *)arg;
    add_client(cli);
	printf("Client %d\n", cli->sock_fd);
	
	char recv_buffer[RECV_BUFFER_SIZE] = {0};

	while(1)
	{
		receive(cli->sock_fd, recv_buffer);
		printf("%s", recv_buffer);
        if(0 == strcmp(recv_buffer, "exit\n") || 0 == strcmp(recv_buffer, "quit\n"))
        {
            break;
        }
		else if(check_key(recv_buffer, "_name_") == 1)
		{
			set_name(cli, recv_buffer);
		}
		else if(check_key(recv_buffer, "_out_") == 1)
		{
			break;
		}
		else
		{
			broadcast(recv_buffer, cli);
		}
		memset(recv_buffer, 0, sizeof(recv_buffer));
	}

	broadcast("has left\n", cli);
	close(cli->sock_fd);
	printf("Close socket\n");
	remove_client(cli->sock_fd);
	free(cli);
	
	pthread_detach(pthread_self());
}

int main()
{
	int serv_fd, conn_fd;
	struct sockaddr_in serv_addr, conn_addr;
	int serv_len, conn_len = sizeof(conn_addr);

	serv_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(0 == serv_fd)
	{
		printf("Socket create failed\n");
		perror("Error");
		exit(0);
	}

	serv_addr.sin_family = AF_INET;
	/* serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); */
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(PORT);
	serv_len = sizeof(serv_addr);

	if(bind(serv_fd, (struct sockaddr *)&serv_addr, serv_len) < 0)
	{
		printf("Socket bind failed\n");
		perror("Error");
		exit(0);
	}

	if(listen(serv_fd, 5) < 0)
	{
		printf("Socket listen failed\n");
		perror("Error");
		exit(0);
	}
	printf("Socket listen on port %d\n", PORT);

    pthread_t thread_id;
	while(1)
	{
		conn_fd = accept(serv_fd, (struct sockaddr *)&conn_addr, (socklen_t *)&conn_len);
		if(conn_fd < 0)
		{
			perror("In accept\n");
			exit(0);
		}
		
        client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->addr = conn_addr;
		cli->sock_fd = conn_fd;
		strcpy(cli->name, "Unknown");
	
		pthread_create(&thread_id, NULL, handler, (void *)cli);
	}

	return 0;
}
