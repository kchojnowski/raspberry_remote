#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/un.h>
#include <pthread.h>

unsigned char finish_flag = 0;


struct sockets {
	int socket_in_fd;
	int socket_un_fd;
	int connection_in_fd;
};

struct rx_thread_args {
    int port;
	struct sockets* sockets;
};

struct tx_thread_args {
	char* path;
	struct sockets* sockets;
};

void* rx_thread(void* arg)
{
	struct rx_thread_args *thread_args = (struct rx_thread_args*)arg;
	struct sockaddr_in address_in;
	socklen_t addr_len;
	unsigned char buffer[256];
	int nbytes;

	
	thread_args->sockets->socket_in_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(thread_args->sockets->socket_in_fd < 0)
	{
		printf("socket() in failed\n");
		return NULL;
	}

	memset(&address_in, 0, sizeof(struct sockaddr_in));

	address_in.sin_family = AF_INET;
	address_in.sin_addr.s_addr = INADDR_ANY;
	address_in.sin_port = htons(thread_args->port);
	addr_len = sizeof(address_in);

	if(bind(thread_args->sockets->socket_in_fd, (struct sockaddr *) &address_in, sizeof(struct sockaddr_in)) != 0)
	{
	    printf("bind() in failed [%d]\n",errno);
		return NULL;
	}

	if(listen(thread_args->sockets->socket_in_fd, 5) != 0)
	{
		printf("listen() in failed\n");
		return NULL;
	}

	thread_args->sockets->connection_in_fd = accept(thread_args->sockets->socket_in_fd, (struct sockaddr *) &address_in, &addr_len);
	if(thread_args->sockets->connection_in_fd < 0)
	{
		printf("accept() failed %d\n", errno);
		return NULL;
	}
	printf("client_net: in socket ready\n");
	while(!finish_flag)
	{
		nbytes = read(thread_args->sockets->connection_in_fd, buffer, 256);
		if(nbytes > 0 && thread_args->sockets->socket_un_fd >= 0)
		{
			write(thread_args->sockets->socket_un_fd, buffer, nbytes);
		} 
	}
	return NULL;
}

void* tx_thread(void* arg)
{

	struct tx_thread_args *thread_args = (struct tx_thread_args*)arg;
	struct sockaddr_un address_un;
	unsigned char buffer[256];
	int nbytes;

	thread_args->sockets->socket_un_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(thread_args->sockets->socket_un_fd < 0)
	{
		printf("socket() un failed\n");
		return NULL;
	}

	memset(&address_un, 0, sizeof(struct sockaddr_un));

    address_un.sun_family = AF_UNIX;
	snprintf(address_un.sun_path, 108, thread_args->path);

	if(connect(thread_args->sockets->socket_un_fd, (struct sockaddr *) &address_un, sizeof(struct sockaddr_un)) != 0) {
	    printf("connect() un failed\n");
		return NULL;
	}
	printf("client_net: un socket ready\n");
	while(!finish_flag)
	{
		nbytes = read(thread_args->sockets->socket_un_fd, buffer, 256);
		printf("Something in UN?\n");
		if(nbytes > 0 && buffer[0] == 255) {
			finish_flag = 1;
			break;
		}

		if(nbytes > 0 && thread_args->sockets->connection_in_fd >=0)
		{
			write(thread_args->sockets->connection_in_fd, buffer, nbytes);
		}
	}
	return NULL;
}

int main(int argc, char** argv)
{
	struct sockets sockets;
	struct sockaddr_un address_un;
	int err = 0;
	
	//creating threads
	struct rx_thread_args rx_targs;
	rx_targs.port = atoi(argv[1]);
	rx_targs.sockets = & sockets;

	struct tx_thread_args tx_targs;
	tx_targs.path = argv[2];
	tx_targs.sockets = &sockets;

	pthread_t rx_thread_id;
	pthread_t tx_thread_id;

	pthread_create(&rx_thread_id, NULL, &rx_thread, &rx_targs);
	pthread_create(&tx_thread_id, NULL, &tx_thread, &tx_targs);

	pthread_join(rx_thread_id, NULL);
	pthread_join(tx_thread_id, NULL);

	close(sockets.socket_un_fd);
	close(sockets.socket_in_fd);
	close(sockets.connection_in_fd);
	return 0;
}
