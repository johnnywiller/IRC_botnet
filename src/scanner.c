#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#define TELNET_PORT 23

struct telnet_desc {
	int fd;
	char *ip;
};

static struct telnet_desc * telnet_connect(char *ip);
static void telnet_login(struct telnet_desc *desc);
static void telnet_login2(struct telnet_desc *desc);
static void terminal_set(int telnet_fd);

int main(int argc, char *argv[]) {

	struct telnet_desc *desc;
	desc = telnet_connect(argv[1]);

	if (desc == NULL) 
		exit(EXIT_FAILURE);

	printf("connected\n");
//	telnet_login(desc);
	telnet_login3(desc);
	printf("conectou\n");

}

static struct telnet_desc * telnet_connect(char *ip) {

	struct telnet_desc *desc;
	struct sockaddr_in *addr;
	int fd;

	addr = calloc(1, sizeof(struct sockaddr_in));
	//memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = inet_addr(ip);
	addr->sin_port = htons(TELNET_PORT);

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket telnet connect");
		return NULL;
	}

	if (connect(fd, (struct sockaddr *) addr, sizeof(struct sockaddr_in)) == -1) {
		perror("connect telnet connect");
		return NULL;
	}
	free(addr);

	desc = malloc(sizeof(struct telnet_desc));
	desc->fd = fd;
	desc->ip = malloc(strlen(ip) + 1);
	strcpy(desc->ip, ip);
	return desc;
}

static void telnet_login3(struct telnet_desc *desc) {

	char negotiation_1[] = {0xFF,0xFC,0X18,0xFF,0xFC,0x20,0xFF,0xFC,0x23,0xFF,0xFC,0x27,0xFF,0xFD,0x03};
	char negotiation_2[] = {0xFF,0xFC,0x01,0xFF,0xFC,0x1F,0xFF,0xFE,0x05,0xFF,0xFB,0x21};
	char telnet_login_name[] = "root\r\n";
	char telnet_login_password[] = "test\r\n";
	char set_command[] = "set watchdog off\r\n";
	char save_command[] = "save\r\n";
	char confirm[] = "y\n";
	char escape[] = {0x5E,0x5D,0x0A}; // ^]
	char quit[] = "quit\n";

	sleep(2);
	write(telnetsock,&negotiation_1[0],15);
	sleep(2);
	write(telnetsock,&negotiation_2[0],12);
	sleep(2);
	write(telnetsock,&telnet_login_name[0],7);
	sleep(2);
	write(telnetsock,&telnet_login_password[0],10);
	sleep(2);
	write(telnetsock,&set_command[0],18);
	sleep(2);
	write(telnetsock,&save_command[0],6);
	sleep(2);
	write(telnetsock,&confirm[0],2);
	sleep(2);
	write(telnetsock,&escape[0],3);
	sleep(2);
	write(telnetsock,&quit[0],5);
	sleep(2);

}

static void telnet_login2(struct telnet_desc *desc) {
	
	char *buf;
	char *last_read;
	
	bool logged = false;
	bool wait_pass = false;

	int MAX_BUF = 1024;
	
	terminal_set(desc->fd);
	
	last_read = calloc(1, MAX_BUF);
	buf = malloc(MAX_BUF + 1);

	while (1) {
		struct timeval ts;
		ts.tv_sec = 1;
		ts.tv_usec = 0;
		
		int nfds = desc->fd + 1;
		fd_set write_set;
		fd_set read_set;
		
		FD_ZERO(&write_set);
		FD_ZERO(&read_set);

		FD_SET(desc->fd, &write_set);
		FD_SET(desc->fd, &read_set);

		int ready = select(nfds, &read_set, &write_set, NULL, &ts);
		if (ready > 0 && FD_ISSET(desc->fd, &read_set)) {
			
			int num_read = read(desc->fd, buf, MAX_BUF - 1);
			buf[num_read] = '\0';
			
			strncpy(last_read, buf, num_read);
			
			//printf("%s", buf);
			write(STDOUT_FILENO, buf, num_read);
			fflush(stdout);
			
			// check if we've a prompt
			if (strstr(buf, "~ #")) {
				logged = true;
				wait_pass = false;
			} 
			sleep(1);
		}
		
		if (FD_ISSET(desc->fd, &write_set)) {
			
			if (!last_read) continue;
			if (wait_pass) continue;

			if (logged) {
				write(desc->fd, "ls\r\n", 4);
				
			} else if (strcasestr(last_read, "login")) {
				// trying username
				write(desc->fd, "root\n", 5);
			}
			if (strcasestr(last_read, "Password")) {
				write(desc->fd, "test\r\n", 6);
				wait_pass = true;
			}	
	
			last_read[0] = '\0';
			sleep(1);
		}
	}	

}

static void terminal_set(int telnet_fd) {

	struct termios term;

	tcgetattr(telnet_fd, &term);
	cfmakeraw(&term);
       	tcsetattr(telnet_fd, TCSANOW, &term);	

}

static void telnet_login(struct telnet_desc *desc) {

	int num_read;
	char *buf = malloc(1024);

	int i = 0;
	printf("teste\n");	
	while((num_read = read(desc->fd, buf, 1024)) > 0) {
		write(STDOUT_FILENO, buf, num_read);
	//	write(STDOUT_FILENO, "aqui", 5);
		fflush(stdout);
	}

	write(STDOUT_FILENO, "teste2\n", 8);
	sleep(0.5);
	write(desc->fd, "root\r\n", 6);
	sleep(0.5);
	
	while((num_read = read(desc->fd, buf, 1024)) > 0) {
		write(STDOUT_FILENO, buf, num_read);
		fflush(stdout);
	}
	
	write(desc->fd, "root\r\n", 6);
	sleep(0.5);
	write(desc->fd, "ls ../\r\n", 8);

	sleep(0.5);

	while((num_read = read(desc->fd, buf, 1024)) > 0) {
		write(STDOUT_FILENO, buf, num_read);
		fflush(stdout);
	}
	
}




