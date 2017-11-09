#include "../header/header.h"


int main(int argc, char *argv[]) {
	char *ch_name = NULL;
	char *nick = NULL;
	char *hostname = NULL;
	char *port = NULL;
	int opt;

	// generate unique seed for this bot
	long long int seed;
	read(open("/dev/urandom", O_RDONLY), &seed, sizeof(seed));
	srand(seed | time(NULL));

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
			port = malloc(strlen(optarg) + 1);
			strcpy(port, optarg);
			break;
		case 'n':
			nick = malloc(strlen(optarg) + 1);
			strcpy(nick, optarg);
			break;
		case 'c':
			ch_name = malloc(strlen(optarg) + 1);
			strcpy(ch_name, optarg);
			break;
		}
	}

	irc_info *info = malloc(sizeof(irc_info));

	if (hostname)
		info->hostname = hostname;

	if (port)
		info->port = atoi(port);
	else
		info->port = IRC_PORT;

	if (nick)
		info->nick = nick;

	if (ch_name)
		info->ch = ch_name;
	else
		info->ch = IRC_CHANNEL;

	printf("before connect\n");
	if (irc_connect(info)) {

		printf("falhou connect\n");
		exit(EXIT_FAILURE);

	}

	printf("before login\n");
	if (irc_login(info)) {

	}

	printf("before listen\n");
	if (irc_listen(info)) {


	}

	return EXIT_SUCCESS;
}
