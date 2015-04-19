#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char** argv) {

	if(argc != 3) {
		printf("Usage: client_pwm pin_number socket_name\n");
		return 0;
	}

	char* gpio_base_path = "/sys/class/gpio/gpio";
	char* gpio_value_file = "value";

	char gpio_value_file_path[64];
	
	sprintf(gpio_value_file_path, "%s%s/%s",gpio_base_path, argv[1], gpio_value_file);

	int fd;
	
	if((fd = open(gpio_value_file_path, O_RDWR)) < 0) {
		printf("Error opening file for write: %s\n",gpio_value_file_path);
		return 1;
	}
	
	//configure and connect to socket
	struct sockaddr_un address;
	int  socket_fd, nbytes;
	unsigned char buffer[256];

	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(socket_fd < 0) {
		printf("socket() failed\n");
		return 1;
	}
	
	memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = AF_UNIX;
	snprintf(address.sun_path, 108, argv[2]);

    if(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0) {
	    printf("connect() failed\n");
	    return 1;
	}
	
	char state_high = '1';
    char state_low = '0';

	while(1) {
	
		nbytes = read(socket_fd, buffer, 256);
		if(nbytes > 0) {
			if(buffer[0] == 255) {
				break;
			} else if(buffer[0] == 0) 
			{
				if(write(fd, &state_low, sizeof(char)) <= 0) 
				{
		            printf("Error writing file");
		            return 1;
				}
			} else
			{
				if(write(fd, &state_high, sizeof(char)) <= 0)                
                {
					printf("Error writing file");
					return 1;
				}

			}
		}
	}

	if(close(fd)<0) {
		printf("Error closing file: %s\n",gpio_value_file_path);
		return 1;
	}

	close(socket_fd);


	return 0;
}
