#ifndef IRC_H
#define IRC_H

#define DEFAULT_NP_CHG 100

typedef struct {
	int fd_irc;
	char *ch;
	char *nick;
	char *hostname;
	uint16_t port;
} irc_info;


int irc_connect(irc_info *info);

int irc_login(irc_info *info);

int irc_send(char *msg, int len, irc_info *info);

int irc_listen(irc_info *info);

char * random_nick();

void send_pong(irc_info *info);

#endif
