#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

struct telnet_desc {
	int fd;
	char *ip;
};

static struct telnet_desc * telnet_connect(char *ip);
static void telnet_login(struct telnet_desc *desc);

int main(int argc, char *argv[]) {

	struct telnet_desc *desc;
	desc = telnet_connect(argv[1]);

	printf("pegou file descriptor = %d\n", desc->fd);

	telnet_login(desc);

	printf("conectou\n");

}

static struct telnet_desc * telnet_connect(char *ip) {

	struct telnet_desc *desc;
	struct sockaddr_in *addr;
	int fd;

	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = inet_addr(ip);
	addr->sin_port = htons(23);

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket telnet connect");
		return NULL;
	}

	if (connect(fd, (struct sockaddr *) addr, sizeof(struct sockaddr_in)) == -1) {
		perror("connect telnet connect");
		return NULL;
	}

	desc = malloc(sizeof(struct telnet_desc));
	desc->fd = fd;

	return desc;
}

static void telnet_login(struct telnet_desc *desc) {

	int num_read;
	char *buf = malloc(1024);

	while ((num_read = read(desc->fd, buf, 1024)) > 0) {

		write(STDOUT_FILENO, buf, num_read);

	}
}




