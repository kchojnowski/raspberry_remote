#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

struct net_thread_args
{
	int *net_socket_fd;
	int *net_connection_fd;
	int *pwm_connection_fd;
	int *gpio_connection_fd;
	char* net_path;
};

struct pwm_thread_args
{
	int *pwm_socket_fd;
	int *pwm_connection_fd;
	char* pwm_path;
};

struct gpio_thread_args
{
    int *gpio_socket_fd;
	int *gpio_connection_fd;
	char* gpio_path;
};

unsigned char finish_flag = 0;


int createUnSocketServer(int* socket_fd, int*connection_fd, char* path)
{
	struct sockaddr_un address;
	socklen_t address_length = sizeof(struct sockaddr_un);

	*socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(*socket_fd < 0)
	{
		printf("socket() failed %s\n", path);
		return 1;
	}

	unlink(path);
	
	memset(&address, 0, sizeof(struct sockaddr_un));

	address.sun_family = AF_UNIX;
	snprintf(address.sun_path, 255, path);

	if(bind(*socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0)
	{
		printf("bind() failed %s\n", path);
		return 1;
	}

	if(listen(*socket_fd, 5) != 0)
	{
		printf("listen() failed %s\n", path);
		return 1;
	}
	
	*connection_fd = accept(*socket_fd, (struct sockaddr *) &address, &address_length);
	if(*connection_fd < 0)
	{
		printf("accept() failed %d %s\n", errno, path);
		return 1;
	}

	return 0;
}

void* net_thread(void *arg)
{
    struct net_thread_args *net_args = (struct net_thread_args*)arg;
	unsigned char buffer[256];
	int nbytes;

	createUnSocketServer(net_args->net_socket_fd, net_args->net_connection_fd, net_args->net_path);

	while(!finish_flag)
	{
		nbytes = read(*net_args->net_connection_fd, buffer, 256);
		if(nbytes >= 2 )
		{
			switch(buffer[0])
			{
				case(1):	//PWM command
					if(*net_args->pwm_connection_fd >= 0) {
						if(write(*net_args->pwm_connection_fd, &buffer[1], nbytes-1) < 0)
							printf("pwm writing %d\n", errno);
					}
					break;
				case(2):    //GPIO command
					if(*net_args->gpio_connection_fd >= 0) {
						if(write(*net_args->gpio_connection_fd, &buffer[1], nbytes-1) < 0)
							printf("gpio writing %d\n", errno);
					}
					break;
			}
		}
	}
	return NULL;
}


void* pwm_thread(void *arg)
{
    struct pwm_thread_args *pwm_args = (struct pwm_thread_args*)arg;
	unsigned char buffer[256];
	int nbytes;

	createUnSocketServer(pwm_args->pwm_socket_fd, pwm_args->pwm_connection_fd, pwm_args->pwm_path);

	while(!finish_flag)
	{
		nbytes = read(*pwm_args->pwm_connection_fd, buffer, 256);
		printf("pwm_thread: %d\n", nbytes);

	}
}

void* gpio_thread(void *arg)
{
    struct gpio_thread_args *gpio_args = (struct gpio_thread_args*)arg;
	unsigned char buffer[256];
	int nbytes;

	createUnSocketServer(gpio_args->gpio_socket_fd, gpio_args->gpio_connection_fd, gpio_args->gpio_path);

    while(!finish_flag)
    {
		nbytes = read(*gpio_args->gpio_connection_fd, buffer, 256);
		printf("gpio_thread: %d\n", nbytes);
    }
}

int main(int argc, char** argv)
{
	unsigned char buffer[256]; 
	int net_socket_fd;
	int net_connection_fd;
	int pwm_socket_fd;
	int pwm_connection_fd;
	int gpio_socket_fd;
	int gpio_connection_fd;

    struct net_thread_args net_args;
	net_args.net_socket_fd = &net_socket_fd;
	net_args.net_connection_fd = &net_connection_fd;
	net_args.net_path = argv[1];
	net_args.pwm_connection_fd = &pwm_connection_fd;
	net_args.gpio_connection_fd = &gpio_connection_fd;

    struct pwm_thread_args pwm_args;
	pwm_args.pwm_socket_fd = &pwm_socket_fd;
	pwm_args.pwm_connection_fd = &pwm_connection_fd;
	pwm_args.pwm_path = argv[2];
	
    struct gpio_thread_args gpio_args;
	gpio_args.gpio_socket_fd = &gpio_socket_fd;
	gpio_args.gpio_connection_fd = &gpio_connection_fd;
	gpio_args.gpio_path = argv[3];
	
	pthread_t net_tid;	
	pthread_t pwm_tid;
	pthread_t gpio_tid;

    pthread_create(&net_tid, NULL, &net_thread, &net_args);
    pthread_create(&pwm_tid, NULL, &pwm_thread, &pwm_args);
    pthread_create(&gpio_tid, NULL, &gpio_thread, &gpio_args);

	pthread_join(net_tid, NULL);
	pthread_join(pwm_tid, NULL);
	pthread_join(gpio_tid, NULL);

	close(net_connection_fd);
	close(pwm_connection_fd);
	close(gpio_connection_fd);

	close(net_socket_fd);
	close(pwm_socket_fd);
	close(gpio_socket_fd);

	unlink(argv[1]);
	unlink(argv[2]);
	unlink(argv[3]);

	return 0;
}
