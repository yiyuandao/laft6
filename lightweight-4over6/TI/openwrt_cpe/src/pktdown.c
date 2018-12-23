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

int skbuf_handle(struct sk_buff* skb)
{
	struct ethhdr* eth;
	int ret = 0;
	
	eth = (struct ethhdr* )eth_hdr(skb);
	if (eth == NULL)
	{
		Log(LOG_LEVEL_ERROR, "eth_hdr-->NULL");
		return -1;
	}
	//filter the ipv6 packet
	//if (htons(eth->h_proto) == ETH_P_IPV6)
	if (skb->data[0] == 0x60)
	{
		ret = ipv6handle(skb);
		return ret;
	}
	
	return 1;	
}


//handle ipv6 packets
int ipv6handle(struct sk_buff* skb)
{
	struct ipv6hdr* iph_v6 = NULL;
	unsigned short offset = 0;
	struct sk_buff *newskb = NULL;
	unsigned char mf = 0;
	unsigned short length = 0;

	iph_v6 = ipv6_hdr(skb);
	if (iph_v6 == NULL)
	{
		Log(LOG_LEVEL_ERROR, "ipv6_hdr-->NULL");
		return -1;
	}

	//filter outgoing ip-in-ipv6 packets
	if (memcmp((void *)iph_v6->saddr.in6_u.u6_addr8,  (void *)laft_info.IPv6LocalAddr, 16) == 0 && iph_v6->nexthdr == 0x04)
	{
		return 1;
	}
	//filter the loacl ipv6 packets
	if (memcmp((void *)iph_v6->daddr.in6_u.u6_addr8,  (void *)laft_info.IPv6LocalAddr, 16) != 0)
	{
		return 1;
	}
	switch(iph_v6->nexthdr)
	{
	//the type of ipinip packets
	case 0x04:
		{
			Log(LOG_LEVEL_NORMAL, "recv IP in ipv6  packet!");
			ipiphandle(skb);
			patch_tcpmss(skb->data, skb->len);
			if (nathandle(skb) < 0)
				return -1;

			return 0;
		}
		break;
	//the type of ipinip fragment packets
	case NEXTHDR_FRAGMENT:
		{
			Log(LOG_LEVEL_NORMAL, "recv IPv6 fragment packet!");
			if (isfragipip(skb) == 0)
				return 1;

			offset = getfragoffset(skb, &mf, &length);
			newskb = fragsk_bufcopy(skb);
			if (newskb == NULL)
				return -1;
			do
			{
				PFRAG_CB pfragcb = NULL;
				struct _tag_frag_head * fraghead = NULL;
				
				pfragcb = kmalloc(sizeof(FRAG_CB), GFP_ATOMIC);
				if (pfragcb == NULL)
				{
					dev_kfree_skb(newskb);
					return -1;
				}
				pfragcb->mf = mf;
				pfragcb->skb = newskb;
				pfragcb->offset = offset;
				pfragcb->length = length;
				fraghash_lock();
				fraghead = fragnodeinsert(pfragcb);
				if (fraghead != NULL)
				{	
					Log(LOG_LEVEL_NORMAL, "the packet is reassembly!");
					fragprocess(fraghead);
				}
				fraghash_unlock();
			}while(0);
			
			return -1;
		}
		break;
	default:
		//Other ipv6  packet
		break;
	}

	return 1;
}

//handle the type of ipinip packets
void  ipiphandle(struct sk_buff*skb)
{
	struct ethhdr* eth_t = NULL;
	struct ipv6hdr *pipv6hdr = NULL;
	unsigned int payload = 0;
	
	eth_t = eth_hdr(skb);
	eth_t->h_proto = htons(ETH_P_IP);

	pipv6hdr = ipv6_hdr(skb);
	payload = htons(pipv6hdr->payload_len);
	
	memmove((void *)pipv6hdr,
		(void *)((unsigned char *)pipv6hdr + sizeof(struct ipv6hdr)),
		payload);

	//move  pointer of tail
	skb->protocol = htons(ETH_P_IP);
	skb->pkt_type = PACKET_HOST;
	skb->tail -= sizeof(struct ipv6hdr);
	skb->len -= sizeof(struct ipv6hdr);
	
	skb_reset_transport_header(skb);
	skb_set_transport_header(skb, sizeof(struct iphdr));
} 

//create the hash key value
static void createpktdownkey(struct sk_buff* skb, PHASH_DATA_DOWN hashdatdown)
{
	struct tcphdr * ptcphdr = NULL;
	struct udphdr *pudphdr = NULL;
	struct icmphdr *picmphdr = NULL;
	struct iphdr *piphdr = NULL;
	
	piphdr = (struct iphdr *)skb->data;
	
	if (piphdr == NULL)
		return;
	
	hashdatdown->proto = piphdr->protocol;
	hashdatdown->dstip = htonl(piphdr->saddr);
	switch(piphdr->protocol)
	{
	case IPPROTO_TCP:
		{
			ptcphdr = (struct tcphdr *)(skb->data + sizeof(struct iphdr));
			
			hashdatdown->newport = htons(ptcphdr->dest);
		}
		break;
	case IPPROTO_UDP:
		{
			pudphdr = (struct udphdr *)(skb->data + sizeof(struct iphdr));
			hashdatdown->newport = htons(pudphdr->dest);
		}
		break;
	case IPPROTO_ICMP:
		{
			unsigned int extip = 0;
			unsigned short port = 0;
			unsigned char proto = 0;
			
			picmphdr = (struct icmphdr *)(skb->data + sizeof(struct iphdr));
			//ICMP ERROR PROCESS
			if (OuterICMPErr_get(picmphdr,
			&port,
			&extip, 
			&proto) > 0)
			{
				hashdatdown->newport = port;
				hashdatdown->dstip = extip;
				hashdatdown->proto = proto;
				Log( LOG_LEVEL_NORMAL,
					"Port:%d, extip:%x, Protocol:0x%x",
					port,
					extip,
					proto);
			}
			else
			hashdatdown->newport = htons(picmphdr->un.echo.id);
		}
		break;
	default :
		break;
	}
}

//handle the packet nat
int nathandle(struct sk_buff* skb)
{
	PSESSIONTUPLES psesstuples = NULL;
	HASH_DATA_DOWN hashdatdown;
	struct tcphdr * ptcphdr = NULL;
	struct udphdr *pudphdr = NULL;
	struct icmphdr *picmphdr = NULL;
	struct iphdr *piphdr = NULL;
	SESSIONTUPLES sessiontuples;
	
	piphdr = (struct iphdr *)(skb->data);
	
	memset(&hashdatdown, 0, sizeof(hashdatdown));
	createpktdownkey(skb, &hashdatdown);

	sessionlock(&hashlock);
	//find the session node from hash table
	psesstuples = DBKeyHashTableFind(pnathash,
		&hashdatdown,
		sizeof(HASH_DATA_DOWN),
		PKT_DOWN);
	if (psesstuples == NULL)
	{
		Log(LOG_LEVEL_ERROR, "Not found tuple");
		sessionunlock(&hashlock);
		return -1;
	}

	piphdr->daddr = htonl(psesstuples->srcip);

	//ICMP ERROR PROCESS
	if (IsOutICMPErrPacket(piphdr) > 0)
	{
		Log(LOG_LEVEL_NORMAL, "Out ICMP Error Packet");
		picmphdr = (struct icmphdr *)((unsigned char *)piphdr + sizeof(struct iphdr));
		OuterICMPErr_set(picmphdr, psesstuples->srcip, psesstuples->oldport);
		OutICMPErrCheckSum(piphdr);
		sessionunlock(&hashlock);
		return 1;
	}

	psesstuples->con_status |= IPS_SEEN_REPLY;

	Log(LOG_LEVEL_NORMAL, "Protocol:0x%x", psesstuples->prot);
	Log(LOG_LEVEL_NORMAL, "IP:0x%x ---------------> 0x%x", htonl(piphdr->daddr), psesstuples->srcip);
	Log(LOG_LEVEL_NORMAL, "Port:%d ---------------> %d", psesstuples->newport, psesstuples->oldport);
	
	memcpy(&sessiontuples, psesstuples, sizeof(SESSIONTUPLES));
	switch(psesstuples->prot)
	{
	case IPPROTO_TCP:
		{
			ptcphdr = (struct tcphdr *)(skb->data + sizeof(struct iphdr));
			ptcphdr->dest = htons(psesstuples->oldport);
			//track the tcp packet
			if (tcp_pkt_track(skb, psesstuples, IP_CT_DIR_REPLY) < 0)
			{
				Log(LOG_LEVEL_ERROR, "tcp track error");
				sessionunlock(&hashlock);
				return -1;
			}
		}
		break;
	case IPPROTO_UDP:
		{
			pudphdr = (struct udphdr *)(skb->data + sizeof(struct iphdr));
			pudphdr->dest = htons(psesstuples->oldport);
			//track the udp packet
			udp_pkt_track(psesstuples);
		}
		break;
	case IPPROTO_ICMP:
		{
			picmphdr = (struct icmphdr *)(skb->data + sizeof(struct iphdr));
			picmphdr->un.echo.id = htons(psesstuples->oldport);
			//track the icmp packet
			icmp_pkt_track(psesstuples, IP_CT_DIR_REPLY);
		}
		break;
	default :
		sessionunlock(&hashlock);
		return -1;
		break;
	}
	sessionunlock(&hashlock);
	//checksum
	pktchecksum(skb, &sessiontuples);
	
	return 1;
}

//checksum
void pktchecksum(struct sk_buff* skb, PSESSIONTUPLES psessiontuples)
{
	struct tcphdr * ptcphdr = NULL;
	struct icmphdr *picmphdr = NULL;
	struct udphdr *pudphdr = NULL;
	struct iphdr *piphdr = NULL;
	unsigned int nsrc = 0;
	 
	memcpy(&nsrc, laft_info.PubIP, 4);
	
	piphdr = (struct iphdr *)skb->data;
	piphdr->check = csum_incremental_update(piphdr->check, nsrc >> 16, (piphdr->daddr & 0xffff0000) >> 16);
	piphdr->check = csum_incremental_update(piphdr->check,  nsrc & 0x0000ffff, piphdr->daddr & 0x0000ffff);

	switch(piphdr->protocol)
	{
	case IPPROTO_TCP:
		{
			ptcphdr = (struct tcphdr *)(skb->data + piphdr->ihl * 4);
			ptcphdr->check = csum_incremental_update(ptcphdr->check, nsrc >> 16, (piphdr->daddr & 0xffff0000) >> 16);
			ptcphdr->check = csum_incremental_update(ptcphdr->check,  nsrc & 0x0000ffff, piphdr->daddr & 0x0000ffff);
			ptcphdr->check = csum_incremental_update(ptcphdr->check,  htons(psessiontuples->newport), htons(psessiontuples->oldport));
		}
		break;
	case IPPROTO_UDP:
		{
			pudphdr = (struct udphdr *)(skb->data + piphdr->ihl * 4);
			pudphdr->check = csum_incremental_update(pudphdr->check, nsrc >> 16, (piphdr->daddr & 0xffff0000) >> 16);
			pudphdr->check = csum_incremental_update(pudphdr->check,  nsrc & 0x0000ffff, piphdr->daddr & 0x0000ffff);
			pudphdr->check = csum_incremental_update(pudphdr->check,  htons(psessiontuples->newport), htons(psessiontuples->oldport));
		}
		break;
	case IPPROTO_ICMP:
		{
			picmphdr = (struct icmphdr *)(skb->data + piphdr->ihl * 4);
			picmphdr->checksum = csum_incremental_update(picmphdr->checksum,
				htons(psessiontuples->newport),
				htons(psessiontuples->oldport));
		}
		break;
	default :
		break;
	}
}

//check the type of packet is ipinip
unsigned char isfragipip(struct sk_buff* skb)
{
	struct ipv6hdr* iph_v6 = NULL;
	struct frag_hdr *fragv6hdr = NULL;
	
	iph_v6 = ipv6_hdr(skb);
	if (iph_v6 == NULL)
		return 0;

	fragv6hdr = (struct frag_hdr *)((unsigned char *)iph_v6 + sizeof(struct ipv6hdr));
	//ipinip
	if (fragv6hdr->nexthdr == 0x04)
		return 1;
	
	return 0;
}

//get fragment offset value
unsigned short getfragoffset(struct sk_buff* skb, unsigned char *mf, unsigned short *length)
{
	struct ipv6hdr* iph_v6 = NULL;
	struct frag_hdr *fragv6hdr = NULL;
	unsigned short offset = 0;
	
	iph_v6 = ipv6_hdr(skb);

	fragv6hdr = (struct frag_hdr *)((unsigned char *)iph_v6 + sizeof(struct ipv6hdr));

	offset = htons(fragv6hdr->frag_off) & FRAG_OFFSET;
	*mf = htons(fragv6hdr->frag_off) & FRAG_MORE;
	*length = htons(iph_v6->payload_len) - sizeof(struct frag_hdr);
	return offset;
}


int OuterICMPErr_get(struct icmphdr *picmphdr,
	unsigned short *port,
	unsigned int *extip,
	unsigned char *protocol)
{
	struct iphdr *piphdr = NULL;
	struct tcphdr *ptcphdr = NULL;
	struct udphdr *pudphdr = NULL;
	
	if ((picmphdr->type == ICMP_ECHOREPLY) || (picmphdr->type == ICMP_ECHO))
	{
		Log(LOG_LEVEL_NORMAL, "picmphdr->type :%x", picmphdr->type);
	 	return -1;
	}
	piphdr = (struct iphdr *)((unsigned char *)picmphdr + sizeof(struct icmphdr));
	*extip = htonl(piphdr->daddr);
	*protocol = piphdr->protocol;
	switch (piphdr->protocol)
	{
	case IPPROTO_TCP:
		{
			ptcphdr = (struct tcphdr *)((unsigned char *)piphdr + sizeof(struct iphdr));
			*port = htons(ptcphdr->source);
		}
		break;
	case IPPROTO_UDP:
		{
			pudphdr = (struct udphdr *)((unsigned char *)piphdr + sizeof(struct iphdr));
			*port = htons(pudphdr->source);
		}
		break;
	case IPPROTO_ICMP:
		{
			picmphdr = (struct icmphdr *)((unsigned char *)piphdr + sizeof(struct iphdr));
			*port = htons(picmphdr->un.echo.id);
		}
		break;
	default :
		break;
	}
	return 1;
}

void OuterICMPErr_set(struct icmphdr *picmphdr, 
	unsigned int inip, 
	unsigned short port)
{
	struct iphdr *piphdr = NULL;
	struct tcphdr *ptcphdr = NULL;
	struct udphdr *pudphdr = NULL;
	unsigned int srcip = 0;
	unsigned short srcport = 0;
	
	if ((picmphdr->type == ICMP_ECHOREPLY) || (picmphdr->type == ICMP_ECHO))
	 	return ;

	piphdr = (struct iphdr *)((unsigned char *)picmphdr + sizeof(struct icmphdr));
	srcip = htonl(piphdr->saddr);
	piphdr->saddr = htonl(inip);
	switch (piphdr->protocol)
	{
	case IPPROTO_TCP:
		{
			ptcphdr = (struct tcphdr *)((unsigned char *)piphdr + sizeof(struct iphdr));
			srcport = htons(ptcphdr->source); 
			ptcphdr->source = htons(port);
		}
		break;
	case IPPROTO_UDP:
		{
			pudphdr = (struct udphdr *)((unsigned char *)piphdr + sizeof(struct iphdr));
			srcport = htons(pudphdr->source); 
			pudphdr->source = htons(port);
		}
		break;
	case IPPROTO_ICMP:
		{
			picmphdr = (struct icmphdr *)((unsigned char *)piphdr + sizeof(struct iphdr));
			srcport = htons(picmphdr->un.echo.id); 
			picmphdr->un.echo.id = htons(port);
		}
		break;
	default :
		break;
	}
	OutICMPErrCheckSum(piphdr);
}

int IsOutICMPErrPacket(struct iphdr *piphdr)
{
	struct icmphdr *picmphdr = NULL;
	
	if (piphdr->protocol == IPPROTO_ICMP)
	{	
		picmphdr = (struct icmphdr *)((unsigned char *)piphdr + sizeof(struct iphdr));
		if ((picmphdr->type == ICMP_ECHOREPLY) || (picmphdr->type == ICMP_ECHO))
	 		return -1;
		return 1;
	}
	
	return -1;
}

void OutICMPErrCheckSum(struct iphdr *piphdr)
{	
	struct tcphdr *ptcphdr = NULL;
	struct udphdr *pudphdr = NULL;
	struct icmphdr *picmphdr = NULL;	
	unsigned short attachsize = 0;
	PTCP_PSD_HEADER pTcppsdHdr = NULL;

	//tcp
	if (piphdr->protocol == 0x06)
	{
		ptcphdr = (struct tcphdr *)((unsigned char *)piphdr + sizeof(struct iphdr));
		
		attachsize = htons(piphdr->tot_len) - sizeof(struct iphdr);
		pTcppsdHdr = (PTCP_PSD_HEADER)kmalloc(attachsize + sizeof(TCP_PSD_HEADER), GFP_ATOMIC);
		if (pTcppsdHdr == NULL)
		{
			return ;
		}
		memset(pTcppsdHdr, 0, attachsize + sizeof(TCP_PSD_HEADER));

		  //填充伪TCP头
  	  pTcppsdHdr->destip = piphdr->daddr;
  	  pTcppsdHdr->sourceip = piphdr->saddr;
  	  pTcppsdHdr->mbz = 0;
  	  pTcppsdHdr->ptcl = 0x06;
  	  pTcppsdHdr->tcpl = htons(attachsize);

	  	  //计算TCP校验和
	  piphdr->check = 0;
	  memcpy((unsigned char *)pTcppsdHdr + sizeof(TCP_PSD_HEADER),
	   (unsigned char *)ptcphdr, attachsize);
	  ptcphdr->check = ip_checksum((unsigned short *)pTcppsdHdr,
	   attachsize + sizeof(TCP_PSD_HEADER));
	  
	  //计算ip头的校验和
	  piphdr->check = 0;
	  piphdr->check = ip_checksum((unsigned short *)piphdr, sizeof(struct iphdr));
	  kfree((void*)pTcppsdHdr);
	  return ;
	}
	//udp
	if (piphdr->protocol == 0x11)
	{
		  PUDP_PSD_HEADER pudp_psd_header=NULL;
		  pudphdr = (struct udphdr *)((unsigned char *)piphdr + sizeof(struct iphdr));
		  attachsize = htons(piphdr->tot_len) - sizeof(struct iphdr);
		  pudp_psd_header = (PUDP_PSD_HEADER)kmalloc(attachsize+sizeof(UDP_PSD_HEADER), GFP_ATOMIC);
		  if (pudp_psd_header == NULL)
		  	return;
		  memset(pudp_psd_header, 0, attachsize + sizeof(UDP_PSD_HEADER));
		   
		  //填充伪UDP头
		  pudp_psd_header->destip = piphdr->daddr;
		  pudp_psd_header->sourceip = piphdr->saddr;
		  pudp_psd_header->mbz = 0;
		  pudp_psd_header->ptcl = 0x11;
		  pudp_psd_header->udpl = htons(attachsize);
		  
		  //计算UDP校验和
		  pudphdr->check=0;
		  memcpy((unsigned char *)pudp_psd_header + sizeof(UDP_PSD_HEADER),
		   (unsigned char *)pudphdr, attachsize);
		  pudphdr->check = ip_checksum((unsigned short *)pudp_psd_header,
		   attachsize + sizeof(UDP_PSD_HEADER));
		
		 	  //计算ip头的校验和
		  piphdr->check= 0;
		  piphdr->check = ip_checksum((unsigned short *)piphdr, sizeof(struct iphdr));
		  
		 kfree((void *)pudp_psd_header);
	  	 return;
	}
	
	if (piphdr->protocol == 0x1)//ICMP
	{
		picmphdr = (struct icmphdr *)((unsigned char *)piphdr + sizeof(struct iphdr));
		//print_buf((unsigned char *)picmphdr, htons(piphdr->tot_len) - sizeof(struct iphdr));

		picmphdr->checksum = 0;
		picmphdr->checksum = ip_checksum((unsigned short *)picmphdr, 
			htons(piphdr->tot_len) - sizeof(struct iphdr));
		//print_buf((unsigned char *)picmphdr, htons(piphdr->tot_len) - sizeof(struct iphdr));
	}
	    
	//计算ip头的校验和
	piphdr->check = 0;
	piphdr->check = ip_checksum((unsigned short *)piphdr, sizeof(struct iphdr));  
}


