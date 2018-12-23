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
#include "pseudo.h"

int construct_eth_header(struct sk_buff* skb)
{
#if 0
	struct ethhdr* header = (struct ethhdr*)skb->data;

	char dst_mac[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
	char src_mac[6] = {0x00,0x00,0x00,0x00,0x00,0x00};

	memcpy(header->h_dest, dst_mac, 6);
	memcpy(header->h_source, src_mac, 6);

	header->h_proto = htons(0x86dd);
#endif

	return 0;
}

int construct_ipv6_header(struct sk_buff* skb, unsigned short len, unsigned char nexthdr)
{
	struct ipv6hdr* ip6h = (struct ipv6hdr*)(skb->data);

	ip6h->priority = 0;
	ip6h->version = 6;

	ip6h->flow_lbl[0] = 0;
	ip6h->flow_lbl[1] = 0;
	ip6h->flow_lbl[2] = 0;

	ip6h->payload_len = htons(len);
	ip6h->nexthdr = nexthdr;
	ip6h->hop_limit = 0x40;

	memcpy(&ip6h->saddr, laft_info.IPv6LocalAddr, 16);
	memcpy(&ip6h->daddr, laft_info.IPv6RemoteAddr, 16);

	return 0;
}

void construct_frag_header(struct sk_buff* skb, unsigned short frag_off, unsigned int ident)
{
	struct frag_hdr *ip6f = (struct frag_hdr*)(skb->data + IP6_HLEN);

	ip6f->nexthdr = 0x04;
	ip6f->reserved = 0;
	ip6f->frag_off = htons(frag_off);
	ip6f->identification = htonl(ident);

}

struct sk_buff *alloc_new_skbuff(unsigned short len)
{
	struct sk_buff *new_skb;
	struct ethhdr* eth = NULL;

	new_skb = dev_alloc_skb(len + 5);
	if(!new_skb)
	{
		Log(LOG_LEVEL_ERROR, "low memory....");
		return NULL;
	}

	skb_reserve(new_skb, 2);
	skb_put(new_skb, len);
	skb_reset_mac_header(new_skb);

	eth = (struct ethhdr*)eth_hdr(new_skb);
	eth->h_proto = htons(ETH_P_IPV6);

	skb_pull(new_skb, ETH_HLEN);
	skb_reset_network_header(new_skb);

	return new_skb;
}

void construct_ipv6_payload(struct sk_buff *new_skb, struct sk_buff *skb, unsigned short offset)
{
	memcpy(new_skb->data + offset, skb->data, skb->len);
}

// replace the src ip and compute checksum
void update_network_header(struct sk_buff *new_skb, struct sk_buff *skb, unsigned short offset)
{
	struct iphdr *iph;
	unsigned char public_ip[4] = {0};

	// nat in iphdr, change the src addr
	memcpy(new_skb->data + offset + 12, laft_info.PubIP, 4); 

	iph = (struct iphdr *)(new_skb->data + offset);

	// compute ip header checksum
	ip_send_check(iph); 
}

void update_frag_data(struct sk_buff *new_skb, struct sk_buff *skb, unsigned short offset, unsigned short len)
{
	int tmplen = IP6_HLEN + IP6F_HLEN;

	memcpy(new_skb->data + tmplen, skb->data + offset, len);
}

// udp: update the src port and checksum
void update_udp_header(struct sk_buff *new_skb, struct sk_buff *skb, unsigned short offset, unsigned short port)
{
	struct udphdr *udph;
	struct udphdr *oldudph;
	struct iphdr *iph;
	//unsigned int udphoff;
	unsigned int nsrc = 0;

	//udphoff = ip_hdrlen(skb);
	iph = ip_hdr(skb);

	// nat in tcphdr, change the src port
	memcpy(new_skb->data + IP6_HLEN + (iph->ihl << 2) + offset, &port, 2); 

	udph = (struct udphdr *)(new_skb->data + IP6_HLEN + (iph->ihl << 2) + offset);
	oldudph = (struct udphdr *)(skb->data + (iph->ihl << 2));

	memcpy(&nsrc, laft_info.PubIP, 4);

	// compute udp checksum
	udph->check = csum_incremental_update(udph->check, (iph->saddr & 0xffff0000) >> 16, nsrc >> 16);
	udph->check = csum_incremental_update(udph->check, (iph->saddr & 0x0000ffff), nsrc & 0x0000ffff);
	udph->check = csum_incremental_update(udph->check, oldudph->source, port);

}

// tcp: replace the src port and compute checksum
void update_tcp_header(struct sk_buff *new_skb, struct sk_buff *skb, unsigned short offset, unsigned short port)
{
	struct tcphdr *tcph;
	struct tcphdr *oldtcph;
	struct iphdr *iph;
	int tcplen = 0;
	unsigned int nsrc = 0;

	iph = ip_hdr(skb);

	memcpy(new_skb->data  + IP6_HLEN + (iph->ihl << 2) + offset, &port, 2); // nat in tcphdr, change the src port

	oldtcph = (struct tcphdr *)(skb->data + (iph->ihl << 2));
	tcph = (struct tcphdr *)(new_skb->data + IP6_HLEN + (iph->ihl << 2) + offset);

	//tcplen = ntohs(iph->tot_len) - (iph->ihl << 2);

	memcpy(&nsrc, laft_info.PubIP, 4);
	//Log(LOG_LEVEL_ERROR, "0000000000000000000  0x%x....", nsrc);
	//Log(LOG_LEVEL_ERROR, "000000port000000000  0x%x....", port);

#if 1
	// compute tcp checksum
	tcph->check = csum_incremental_update(tcph->check, (iph->saddr & 0xffff0000) >> 16, nsrc >> 16);
	tcph->check = csum_incremental_update(tcph->check, (iph->saddr & 0x0000ffff), nsrc & 0x0000ffff);
	tcph->check = csum_incremental_update(tcph->check, oldtcph->source, port);
#endif
#if 0
	skb_pull(new_skb, ETH_HLEN + 40);
	calc_tcp_checksum(new_skb);
	skb_push(new_skb, ETH_HLEN + 40);
#endif
}

// icmp: replace the sequence and compute checksum
void update_icmp_header(struct sk_buff *new_skb, struct sk_buff *skb, unsigned short offset, unsigned short port)
{
	struct iphdr *iph;
	struct icmphdr *icmph;
	struct icmphdr *oldicmph;

	iph = ip_hdr(skb);

	icmph = (struct icmphdr *)(new_skb->data + IP6_HLEN + (iph->ihl << 2) + offset);
	icmph->un.echo.id = port;
	oldicmph = (struct icmphdr *)(skb->data + (iph->ihl << 2));

	icmph->checksum = csum_incremental_update(oldicmph->checksum, oldicmph->un.echo.id, icmph->un.echo.id);
}

int update_transport_header(struct sk_buff *new_skb, struct sk_buff *skb, PSESSIONTUPLES session_tuple, unsigned short offset)
{
	struct iphdr *iph;

	iph = ip_hdr(skb);

	switch(iph->protocol)
	{
		case IPPROTO_TCP:

			update_tcp_header(new_skb, skb, offset, htons(session_tuple->newport));

			if (tcp_pkt_track(skb, session_tuple, IP_CT_DIR_ORIGINAL) < 0)
			{
				dev_kfree_skb_any(new_skb);
				return -1;
			}

			break;
		case IPPROTO_UDP:

			update_udp_header(new_skb, skb, offset, htons(session_tuple->newport));
			udp_pkt_track(session_tuple); 

			break;
		case IPPROTO_ICMP:

			update_icmp_header(new_skb, skb, offset, htons(session_tuple->newport));
			icmp_pkt_track(session_tuple, IP_CT_DIR_ORIGINAL); 

			break;
		default:
			break;
	}

	return 0;
}

void set_new_skb_attr(struct sk_buff *new_skb, struct sk_buff *skb)
{
	//struct ethhdr* eth = NULL;

	//skb_reset_mac_header(new_skb);

	//eth = (struct ethhdr*)eth_hdr(skb);
	//set_source_mac(new_skb, eth->h_dest);

	//eth = (struct ethhdr*)eth_hdr(new_skb);
	//eth->h_proto = htons(ETH_P_IPV6);

	//skb_pull(new_skb, ETH_HLEN);
	//skb_reset_network_header(new_skb);

	  
	new_skb->pkt_type = PACKET_HOST;
	new_skb->protocol = htons(ETH_P_IPV6);
	new_skb->dev = skb->dev;
}
