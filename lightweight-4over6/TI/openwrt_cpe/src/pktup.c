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

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  get_hash_data_up
 *  Description:  get nat info(saddr, daddr, protocl, sport or sequence(icmp)) for every packet(stream_up)
 * =====================================================================================
 */
int get_hash_data_up(struct sk_buff *skb, PHASH_DATA_UP up) 
{
	struct iphdr* iph;
	struct tcphdr *tcph;
	struct udphdr *udph;
	struct icmphdr *icmph;

	iph = ip_hdr(skb);
	if(!iph)
	{   
		Log(LOG_LEVEL_ERROR, "iph error");
		return -1; 
	}   

	up->srcip = ntohl(iph->saddr);
	up->dstip = ntohl(iph->daddr);
	up->proto = iph->protocol;

	if (iph->protocol == IPPROTO_TCP)
	{   
		tcph = (struct tcphdr *)(skb->data + sizeof(struct iphdr));
		if (!tcph)
		{   
			Log(LOG_LEVEL_ERROR, "tcph error");
			return -1; 
		}   
		up->oldport = ntohs(tcph->source);
	}   
	else if (iph->protocol == IPPROTO_UDP)
	{   
		udph = (struct udphdr *)(skb->data + sizeof(struct iphdr));
		if (!udph)
		{   
			Log(LOG_LEVEL_ERROR, "udph error");
			return -1; 
		}   
		up->oldport = ntohs(udph->source);
	}   
	else if (iph->protocol == IPPROTO_ICMP)
	{   
		icmph = (struct icmphdr *)(skb->data + sizeof(struct iphdr));
		up->oldport = ntohs(icmph->un.echo.id);
	}   
	else
	{   
		Log(LOG_LEVEL_ERROR, "can't process this protocol: %d", iph->protocol);
		return -1; 
	}   

	return 0;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  get_hash_data_down
 *  Description:  get nat info for stream down packet, mainly get port(seq) for tcp,udp,icmp
 * =====================================================================================
 */
int get_hash_data_down(struct sk_buff *skb, PHASH_DATA_DOWN down)
{
	struct iphdr* iph;
	int port = 0;

	iph = ip_hdr(skb);	
	if(!iph)
	{
		Log(LOG_LEVEL_ERROR, "iph error");
		return -1;
	}

	down->dstip = htonl(iph->daddr);
	down->proto = iph->protocol;

	switch (iph->protocol)
	{
		case IPPROTO_TCP:
			port = get_tcp_port(laft_info.LowPort, laft_info.HighPort);
			if (port < 0)
			{
				Log(LOG_LEVEL_ERROR, "no tcp port: %d---->%u", port, port); 
				return -1;
			}
			down->newport = port;
			break;	

		case IPPROTO_UDP:
			port = get_udp_port(laft_info.LowPort, laft_info.HighPort);
			if (port < 0)
			{
				Log(LOG_LEVEL_ERROR, "no udp port");
				return -1;
			}
			down->newport = port;
			break;	

		case IPPROTO_ICMP:
			down->newport = get_icmp_port(laft_info.LowPort, laft_info.HighPort);
			break;

		default:
			break;
	}

	return 1;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  get_tuple
 *  Description:  record Five-tuple for nat entry
 * =====================================================================================
 */
PSESSIONTUPLES get_tuple(PHASH_DATA_UP up, PHASH_DATA_DOWN down)
{
	PSESSIONTUPLES tuple; 

	tuple = kmalloc(sizeof(SESSIONTUPLES), GFP_ATOMIC);
	if (tuple == NULL)
	{
		Log(LOG_LEVEL_ERROR, "kmalloc error");
		return NULL;
	}

	memset(tuple, 0, sizeof(SESSIONTUPLES));
	tuple->srcip = up->srcip;
	tuple->dstip = up->dstip;
	tuple->oldport = up->oldport;
	tuple->newport = down->newport;
	tuple->prot = up->proto;

	return tuple;
}

int compare_session(void *pKey, void *pData, int type)
{
	PSESSIONTUPLES s = (PSESSIONTUPLES)pData;
	PHASH_DATA_UP p;
	PHASH_DATA_DOWN q;

	if (type == PKT_UP)
	{
		p = (PHASH_DATA_UP)pKey;		

		if (p->proto == s->prot && p->srcip == s->srcip && 
				p->dstip == s->dstip && p->oldport == s->oldport) 
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	if (type == PKT_DOWN)
	{
		q = (PHASH_DATA_DOWN)pKey;		

		if (q->proto == s->prot && q->dstip == s->dstip && q->newport == s->newport)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	return -1;
}

/*
   unsigned int get_mtu(struct sk_buff *skb, const struct net_device *in)
   {
   unsigned int mtu;

   iph = ip_hdr(skb);
   ip_route_input(skb, iph->daddr, iph->saddr, 0, in);
   mtu = dst_mtu(skb_dst(skb));

   printk("get a packet now ----------out put mtu is %d------------------->\n", mtu);

   return mtu;
   }
   */
PSESSIONTUPLES search_nat_hashtable(struct sk_buff* skb, PHASH_DATA_UP up)
{
	PSESSIONTUPLES session_tuple = NULL;
	HASH_DATA_DOWN down;

	memset(&down, 0, sizeof(HASH_DATA_DOWN));

	session_tuple = DBKeyHashTableFind(pnathash, up, sizeof(HASH_DATA_UP), PKT_UP);
	if (session_tuple == NULL)
	{
		if (get_hash_data_down(skb, &down) < 0)
		{
			Log(LOG_LEVEL_ERROR, "no free port !");
			return NULL;
		}

		session_tuple = get_tuple(up, &down);
		if (session_tuple == NULL)
		{
			return NULL;
		}

		if (DBKeyHashTableInsert(pnathash, up, sizeof(HASH_DATA_UP), &down, sizeof(HASH_DATA_DOWN), session_tuple) < 0)
		{
			return NULL;
		}

	}
	else
	{

	}

	return session_tuple;
}

int encap(struct sk_buff *skb, PSESSIONTUPLES session_tuple, unsigned int mtu)
{
	int max_v4packet_len = mtu - IP6_HLEN;
	int ret = 0;
	int frag = 0;

	patch_tcpmss(skb->data, skb->len);
	
	frag = skb_is_nonlinear(skb); 

	// normal packet, no need to frag
	if (frag == 0 && skb->len <= max_v4packet_len)  
	{
		ret = normal_packet_handl(skb, session_tuple);
	}
	else if (frag == 0 && skb->len > max_v4packet_len) 
	{
		// one big packet , split two fragments
		ret = ipv6_frag_handl(skb, session_tuple, mtu);
	}
	else if (frag > 0) 
	{
		if (__skb_linearize(skb) != 0)
		{
			Log(LOG_LEVEL_ERROR, "low memory....\n");
			return -1;
		}
		ret = ipv6_frag_handl(skb, session_tuple, mtu);
	}

	return ret;
}


int normal_packet_handl(struct sk_buff *skb, PSESSIONTUPLES session_tuple)
{
	struct sk_buff *new_skb = NULL;
	int headlen = IP6_HLEN + ETH_HLEN;
	struct ethhdr* eth = NULL;

	new_skb = alloc_new_skbuff(skb->len + headlen);
	if (new_skb == NULL)
	{
		Log(LOG_LEVEL_ERROR, "low memory....");
		return -1;
	}

	construct_ipv6_header(new_skb, skb->len, IPPROTO_IPIP);
	construct_ipv6_payload(new_skb, skb, IP6_HLEN);

	// replace the src ip and checksum
	update_network_header(new_skb, skb, IP6_HLEN); 

	// replace src port and compute checksum
	if (update_transport_header(new_skb, skb, session_tuple, 0) == -1) 
	{
		Log(LOG_LEVEL_ERROR, "update_transport_header");
		return -1;
	}

	set_new_skb_attr(new_skb, skb);
	SENDSKB(new_skb);

	return 0;
}

int ipv6_frag_handl(struct sk_buff *skb, PSESSIONTUPLES session_tuple, unsigned int mtu)
{
	struct sk_buff *new_skb = NULL;
	unsigned max_frag_len;
	unsigned int ident = 0;
	unsigned short len = 0;
	unsigned short left = skb->len;
	unsigned short frag_off = 0;
	unsigned short offset = 0;
	unsigned short data_off =  IP6_HLEN + IP6F_HLEN;

	// align  on an eight byte boundary
	max_frag_len = mtu & ~7; 

	while (left > 0)
	{
		len = left;
		if (len > max_frag_len - 48)
		{
			// exculde ipv6 header and frag header
			len = max_frag_len - 48;
		}

		new_skb = alloc_new_skbuff(len + IP6_HLEN + IP6F_HLEN + ETH_HLEN);
		if (new_skb == NULL)
		{
			Log(LOG_LEVEL_ERROR, "low memory....");
			return -1;
		}

		construct_ipv6_header(new_skb, len + IP6F_HLEN, IPPROTO_FRAGMENT);

		//copy playload
		memcpy(new_skb->data + data_off, skb->data + offset, len);

		// first fragment
		if (ident == 0)
		{
			ident = random32();
			// replace the src ip and checksum
			update_network_header(new_skb, skb, IP6_HLEN + IP6F_HLEN); 

			// replace src port and compute checksum
			if (update_transport_header(new_skb, skb, session_tuple, IP6F_HLEN) == -1)
			{
				Log(LOG_LEVEL_ERROR, "update_transport_header return -1....\n");
				return -1;
			}
		}

		frag_off = offset;
		left -= len;
		if (left > 0)
		{
			frag_off |= 1;
		}
		construct_frag_header(new_skb, frag_off, ident);

		set_new_skb_attr(new_skb, skb);
		//netif_rx(new_skb);
		SENDSKB(new_skb);

		offset += len;
	}// end while

	return 0;
}
