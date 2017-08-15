#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#define SERVER_PORT 3030
#define MAX_PENDING_CONNECTIONS 10
#define MAX_SOCKET_BUF 1024
#define MAX_BUF 1024

#define IRC_HOST "localhost"
#define IRC_PORT "6667"

int irc_connect();
int irc_send(char *msg, int fd);

int main() {

	/*
	int fd_socket, fd_socket_client;
	struct sockaddr_in addr_server, addr_client;

	if ((fd_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("opening socket");
		exit(EXIT_FAILURE);
	}

	memset(&addr_server, 0, sizeof(struct sockaddr_in));

	addr_server.sin_family = AF_INET;
	addr_server.sin_addr.s_addr = INADDR_ANY;
	addr_server.sin_port = htons(SERVER_PORT);

	if (bind(fd_socket, (struct sockaddr*) &addr_server, sizeof(addr_server)) == -1) {
		perror("binding socket");
		exit(EXIT_FAILURE);
	}

	if (listen(fd_socket, MAX_PENDING_CONNECTIONS) == -1) {
		perror("listen socket");
		exit(EXIT_FAILURE);
	}

	socklen_t addr_len = sizeof(struct sockaddr_in);
	if ((fd_socket_client = accept(fd_socket,
		(struct sockaddr*) &addr_client, &addr_len)) == -1) {

		perror("accepting connection");
		exit(EXIT_FAILURE);
	}

	int num_read, msg_len;
	msg_len = strlen("cliente mandou: ");
	char buf[MAX_SOCKET_BUF], str_buf[MAX_SOCKET_BUF + msg_len + 1];
	while((num_read = recv(fd_socket_client, buf, MAX_SOCKET_BUF, 0)) > 0) {
		snprintf(str_buf, num_read + msg_len + 1, "Cliente mandou: %s", buf);
		puts(str_buf);
	}
	*/

	int fd_irc;
	int num_read;
	char buf[MAX_BUF], message[MAX_BUF + 1];

	fd_irc = irc_connect();

	irc_send("HELLO", fd_irc);
	irc_send("NICK bsh33p", fd_irc);
	irc_send("USER johnny 0 * :willer", fd_irc);
	irc_send("JOIN :#botnet-ch", fd_irc);

	while((num_read = read(fd_irc, buf, sizeof(buf))) > 0) {
	       snprintf(message, num_read, "%s", buf);
	       puts(message);
	}


}

int irc_send(char *msg, int fd) {

	if (write(fd, msg, strlen(msg)) == -1) {
		perror("write irc");
		return -1;
	}

	if (write(fd, "\n", 1) == -1) {
		perror("write new line irc");
		return -1;
	}

	return 0;
}

int irc_connect() {

	int fd_irc;
      	struct addrinfo hints, *result, *rp;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
	hints.ai_flags = 0;

	if (getaddrinfo(IRC_HOST, IRC_PORT, &hints, &result) != 0) {
		perror("getaddrinfo");
		exit(EXIT_FAILURE);
	}


	for (rp = result; rp != NULL; rp = rp->ai_next) {
		printf("%d\n", rp->ai_family);
		if ((fd_irc = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
			perror("irc socket");
			//exit(EXIT_FAILURE);
			continue;
		}

		if (connect(fd_irc, rp->ai_addr, rp->ai_addrlen) == -1) {
			perror("irc connect");
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (rp == NULL) {
		close(fd_irc);
		perror("cannot connect to irc");
		return -1;
	}

	return fd_irc;
}









