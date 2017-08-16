#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

#define MAX_BUF 1024
#define IRC_HOST "localhost"
#define IRC_PORT "6667"

struct irc_desc {
	int fd;
	char *ch_name;
};

static int irc_connect();
static int irc_send(char *msg, int fd);
static void irc_login(int fd_irc, char *nick, char *ch_name);
static void *irc_listen(void *arg);
static void *stdin_listen(void *arg);
static int irc_send_priv(char *msg, int fd);

int main() {

	int fd_irc;
	void *res;
	pthread_t t_irc, t_stdin;

	struct irc_desc *desc = malloc(sizeof(struct irc_desc));

	desc->ch_name = "#botnet-ch";

	if ((fd_irc = irc_connect()) == -1) {
		perror("cannot connect to IRC");
		exit(EXIT_FAILURE);
	}

	desc->fd = fd_irc;

	irc_login(fd_irc, "my-bot", "#botnet-ch");

	pthread_create(&t_irc, NULL, irc_listen, &fd_irc);
	puts("thread irc_listen created");

	pthread_create(&t_stdin, NULL, stdin_listen, desc);
	puts("thread stdin_listen created");

	pthread_join(t_irc, &res);
	pthread_join(t_stdin, &res);

}

static void *stdin_listen(void *arg) {

	struct irc_desc *desc = (struct irc_desc*)arg;
	int fd_irc = desc->fd;
	int num_read;
	char buf[MAX_BUF];

	char *msg = malloc(MAX_BUF + strlen(desc->ch_name) + 3);

	while((num_read = read(STDIN_FILENO, buf, sizeof(buf) - 1)) > 0) {
		snprintf(msg, strlen(desc->ch_name) + num_read + 3, "%s :%s", desc->ch_name, buf);
		irc_send_priv(msg, fd_irc);
	}

	free(msg);

}

static void *irc_listen(void *arg) {

	int fd_irc  = *((int*) arg);
	int num_read;
	char buf[MAX_BUF], message[MAX_BUF + 1];

	while((num_read = read(fd_irc, buf, sizeof(buf))) > 0) {
	       snprintf(message, num_read, "%s", buf);
	       puts(message);
	}
}

static void irc_login(int fd_irc, char *nick, char *ch_name) {
	int ret;

	char *HELLO = "HELLO";
	char *NICK = malloc(strlen(nick) + 6);
	char *USER = "USER bot 0 * :bot";
	char *JOIN = malloc(strlen(ch_name) + 6);

	snprintf(NICK, strlen(nick) + 6, "NICK %s", nick);
	snprintf(JOIN, strlen(ch_name) + 6, "JOIN %s", ch_name);

	if ((ret = irc_send(HELLO, fd_irc)) == -1) {
		perror("sending hello");
		exit(EXIT_FAILURE);
	}

	if ((ret = irc_send(NICK, fd_irc)) == -1) {
		perror("sending nick");
		exit(EXIT_FAILURE);
	}
	if ((ret = irc_send(USER, fd_irc)) == -1) {
		perror("sending user");
		exit(EXIT_FAILURE);
	}
	if ((ret = irc_send(JOIN, fd_irc)) == -1) {
		perror("sending join");
		exit(EXIT_FAILURE);
	}

	free(NICK);
	free(JOIN);

}

static int irc_send_priv(char *msg, int fd) {

	char *PRIVMSG = malloc(strlen(msg) + 9);

	snprintf(PRIVMSG, strlen(msg) + 9, "PRIVMSG %s", msg);

	if (write(fd, PRIVMSG, strlen(PRIVMSG)) == -1) {
		perror("write privmsg irc");
		return -1;
	}

	free(PRIVMSG);
	return 0;
}

static int irc_send(char *msg, int fd) {

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

static int irc_connect() {

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









