/*-
 * Copyright (c) 2010-2012, China Telecommunication Corporations
 * All rights reserved.
 *
 * Authors:
 * 		Qiong Sun, Chan Zhao, Bo Li, Daobiao Gong 
 * 		
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the China Telecommunication Corporations nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


#include "precomp.h"

//get tcp status index
static unsigned int get_conntrack_index(struct tcphdr * tcph)
{
	if (tcph->rst ) return 3;
	else if (tcph->syn) return 0;
	else if (tcph->fin) return 1;
	else if (tcph->ack) return 2;
	else return 4;
}

//refresh timer
static void refreshtimer(PSESSIONTUPLES ptuples, long timer)
{
	ptuples->time_out = timer;
}

//delete session node
static void sessiondel(PDBKEYHASHTABLE pTable, PSESSIONTUPLES ptuples, unsigned char type)
{
	HASH_DATA_UP hashdatup;
	HASH_DATA_DOWN hashdatdown;
	
	if (type == PKT_DOWN)
	{
		hashdatdown.proto = ptuples->prot;
		hashdatdown.dstip = ptuples->dstip;
		hashdatdown.newport = ptuples->newport;

		DBKeyHashTableDelete(pTable, &hashdatdown, sizeof(HASH_DATA_DOWN), type);
	}
	else if (type == PKT_UP)
	{
		hashdatup.proto = ptuples->prot;
		hashdatup.oldport = ptuples->oldport;
		hashdatup.dstip = ptuples->dstip;
		hashdatup.srcip = ptuples->srcip;
		
		DBKeyHashTableDelete(pTable, &hashdatup, sizeof(HASH_DATA_UP), type);
	}
}

//track tcp packet
int tcp_pkt_track(struct sk_buff* skb,
	PSESSIONTUPLES ptuples,
	unsigned char  maniptype)
{
	enum tcp_conntrack newconntrack, oldtcpstate;
	struct tcphdr * pTcphdr = NULL;
	struct iphdr *piphdr = NULL;

	piphdr = (struct iphdr *)skb->data;
	pTcphdr = (struct tcphdr *)(skb->data + sizeof(struct iphdr));

	//get tcp old state
	oldtcpstate = ptuples->tcp_cb.state;
	//get tcp new state
	newconntrack = tcp_conntracks[maniptype][get_conntrack_index(pTcphdr)][oldtcpstate];

	if (newconntrack == TCP_CONNTRACK_MAX)
		return -1;
	
	ptuples->tcp_cb.state = newconntrack;
	
	if (oldtcpstate == TCP_CONNTRACK_SYN_SENT
	    && maniptype == IP_CT_DIR_REPLY
	    && (pTcphdr->syn) && (pTcphdr->ack))
	    		ptuples->tcp_cb.handshake_ack = htonl(ntohl(pTcphdr->seq) + 1);
	
	if (!(ptuples->con_status & IPS_SEEN_REPLY) && (pTcphdr->rst))
	{
		//delete the tcp states was rst packet
		setportbit(ptuples);
		sessiondel(pnathash, ptuples, PKT_UP);
	} 
	else
	{
		if ((oldtcpstate == TCP_CONNTRACK_SYN_RECV)
		    && (maniptype == IP_CT_DIR_ORIGINAL)
		    && (pTcphdr->ack)
		    && !(pTcphdr->syn)
		    && (pTcphdr->ack == ptuples->tcp_cb.handshake_ack))
		    //TCP connection was assured
		    	ptuples->con_status |= IPS_ASSURED;
		//refresh timeout
		refreshtimer(ptuples, tcp_timeouts[newconntrack]);
	}
	
	return 1;
}

//track udp packet
void udp_pkt_track(PSESSIONTUPLES ptuples)
{
	if (ptuples->con_status & IPS_SEEN_REPLY) 
	{
		//refresh timeout
		refreshtimer(ptuples, UDP_STREAM_TIMEOUT);
		ptuples->con_status |= IPS_ASSURED;
	} 
	else
		refreshtimer(ptuples, UDP_TIMEOUT);
}

//track icmp packet
void icmp_pkt_track(PSESSIONTUPLES ptuples, 
	 unsigned char maniptype) 
{ 
	if (maniptype == IP_CT_DIR_REPLY)  
	{ 
		if (--ptuples->icmp_seq <= 0)
			sessiondel(pnathash, ptuples, PKT_DOWN);
	}
	else  
	{	
		ptuples->icmp_seq ++;
		refreshtimer(ptuples, ICMP_TIMEOUT); 
	} 
} 

//reset the port bit map
void setportbit(PSESSIONTUPLES psessiontuples)
{
	switch(psessiontuples->prot)
	{
	case 0x06:
		reset_tcp_port(psessiontuples->newport);
		break;
	case 0x11:
		reset_udp_port(psessiontuples->newport);
		break;
	case 0x01:
		break;
	default:
		break;
	}
}
#ifndef MOBILE
// send skb
void Dev_queue_xmit(struct sk_buff *skb)
{
	unsigned short protocol = ntohs(skb->protocol);
	unsigned char *dest_mac = NULL;

	if (protocol == ETH_P_IPV6)
	{    
		dest_mac = lookup_ipv6_dest_mac(skb);
	}    
	else if (protocol == ETH_P_IP)
	{    
		dest_mac = lookup_ipv4_dest_mac(skb);
	}    

	if (dest_mac != NULL)
	{    
		set_dest_mac(skb, dest_mac);
		set_source_mac(skb, skb->dev->dev_addr);
	}    
	else 
	{    
		kfree_skb(skb);
		return;
	}    

	skb_push(skb, ETH_HLEN);
	dev_queue_xmit(skb);
}
#endif
#if 0
void set_source_mac(struct sk_buff* skb, char *mac)
{
	struct ethhdr* eth = NULL;

	eth = (struct ethhdr* )eth_hdr(skb);
	memcpy(eth->h_source, mac, ETH_ALEN);
}
#endif
static void
fix_msscksum(u_char *psum, u_char *pmss, u_short newmss)
{
	int sum;
	int m;

	sum = psum[0] << 8;
	sum |= psum[1];
	sum = ~sum & 0xffff;
	m = pmss[0] << 8;
	m |= pmss[1];
	m = (m >> 16) + (m & 0xffff);
	sum += m ^ 0xffff;
	pmss[0] = newmss >> 8;
	pmss[1] = newmss;
	m = pmss[0] << 8;
	m |= pmss[1];
	sum += m;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += sum >> 16;
	sum = ~sum & 0xffff;
	psum[0] = sum >> 8;
	psum[1] = sum & 0xff;
}



void
patch_tcpmss(unsigned char *buf4, unsigned int len)
{
#define IPHDRLEN	20
#define TCPSEQ		(IPHDRLEN + 4)
#define TCPACK		(IPHDRLEN + 8)
#define TCPOFF		(IPHDRLEN + 12)
#define TCPFLAGS	(IPHDRLEN + 13)
#define TCPCKSUMH	(IPHDRLEN + 16)
#define TCPCKSUML	(IPHDRLEN + 17)

#define TCPHDRLEN	20
#define TCPOFFMSK	0xf0
#define TCPFFIN		0x01
#define TCPFSYN		0x02
#define TCPFRST		0x04
#define TCPFACK		0x10
#define TCPOPTEOL	0
#define TCPOPTNOP	1
#define TCPOPTMSS	2
#define TCPOPTMSSLEN	4
#define TCPOPTMD5	19
#define IP6HDRLEN	40
#define IPPROTO		9
#define IPTCP		6

	unsigned int hl, i, found = 0;
	unsigned short mss;
	
	/* is patching enabled? */
	if (buf4[IPPROTO] != IPTCP)
		return;
	//if ((t->flags & TUNMSSFLG) == 0)
	//	return;
	/* need SYN flag */
	if ((buf4[TCPFLAGS] & TCPFSYN) == 0)
		return;
	hl = (buf4[TCPOFF] & TCPOFFMSK) >> 2;
	/* no data */
	if (hl + IPHDRLEN != len)
		return;
	/* but some options */
	if (hl <= TCPHDRLEN)
		return;
	/* scan option */
	i = IPHDRLEN + TCPHDRLEN;
	while (i < len) {
		if (buf4[i] == TCPOPTEOL) {
			
			break;
		}
		if (buf4[i] == TCPOPTNOP) {
			i++;
			continue;
		}
		if (i + 2 > len) {
			return;
		}
		if (buf4[i + 1] < 2) {
			return;
		}
		if (i + buf4[i + 1] > len) {
			return;
		}
		if (buf4[i] == TCPOPTMD5) {
			return;
		}
		if (buf4[i] == TCPOPTMSS) {
			if (found == 0)
				found = i;
		}
		i += buf4[i + 1];
	}
	if (found == 0) {
		Log(LOG_LEVEL_WARN, "no TCP MSS (after scan)");
		return;
	}
	i = found;
	if (buf4[i + 1] != TCPOPTMSSLEN) {
		Log(LOG_LEVEL_ERROR, "bad TCP MSS option length");
		return;
	}
	i += 2;
	mss = buf4[i] << 8;
	mss |= buf4[i + 1];

	//Log(LOG_LEVEL_NORMAL, "..........mss:%d", mss);
	/* no patch needed */
	if ((mss + IPHDRLEN + TCPHDRLEN) <= (1280 - IP6HDRLEN))
		return;
	
	fix_msscksum(buf4 + TCPCKSUMH, buf4 + i,
		     1280 - (IP6HDRLEN + IPHDRLEN + TCPHDRLEN));
}



