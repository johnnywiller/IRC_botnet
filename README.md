# IRC_botnet
A simple botnet that propagates via TELNET, controlled via IRC server, can execute shell commands and launch DDoS attacks 

This botnet targets TELNET services with common usernames and passwords and is capable of launch DDoS attacks. 
It handle shell commands too, returning the result via IRC channel.

This botnet is tested only in telnet service implemented by BusyBox linux. It's not guarantee success in other implementations

Two types of DDoS is possible UDP flood and TCP SYN flood.

Possible commands that the botmaster can use in IRC channel

  
