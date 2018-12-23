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

static struct sk_buff * dhcp_bufalloc(unsigned int length,
	struct net_device *dev,
	unsigned short usProtocol)
{
	struct sk_buff * new_skb_buf = NULL;
	struct ethhdr* eth_t = NULL;
	

	new_skb_buf = dev_alloc_skb(length + ETH_HLEN + 5);
    if(!new_skb_buf)
       return NULL;
 
    skb_reserve(new_skb_buf, 2);
    skb_put(new_skb_buf, ETH_HLEN + length);
	
	eth_t = (struct ethhdr*)new_skb_buf->data;
	eth_t->h_proto = htons(usProtocol);
	
    new_skb_buf->pkt_type = PACKET_HOST;
    new_skb_buf->protocol = htons(usProtocol);
    new_skb_buf->dev = dev;
	
	skb_reset_mac_header(new_skb_buf);
    skb_pull(new_skb_buf, ETH_HLEN);
		
	return new_skb_buf;
}



int dhcpv6_tx_captrue(struct sk_buff* skb)
{
	struct ipv6hdr* piphv6 = NULL;
	struct udphdr *pudph = NULL;
	unsigned int payloadlen = 0;
	PDHCPV6_HDR pdhcpv6h = NULL;
	PDHCPV6_MSG_HDR pdhcpmsghdr = NULL;
	unsigned short OptId = 0;
	DHCPV6_MSG_NODE Dhcpv6msglist;
	PDHCPV6_MSG_NODE pDhcpMsgNode = NULL, p = NULL, b = NULL;
	unsigned char* pMsg = NULL;
	unsigned int MsgLen = 0, pos = 0, SubMsgLen = 0;
	struct sk_buff* newskb = NULL;

	piphv6 = (struct ipv6hdr*)skb->data;
	if (piphv6 == NULL)
		return -1;
	
	if (piphv6->version != 6)
		return 0;
	
	if (piphv6->nexthdr != 0x11){
		return 0;
	}
	
	pudph = (struct udphdr *)(skb->data + sizeof(struct ipv6hdr));
	if ((pudph->source == htons(0x0222)) && (pudph->dest == htons(0x0223))){
		Log(LOG_LEVEL_NORMAL, "dhcpv6 packet !!!!");
		payloadlen = htons(pudph->len) - sizeof(struct udphdr);
		pdhcpv6h = (PDHCPV6_HDR)((unsigned char *)pudph + sizeof(struct udphdr));	
		if (!((pdhcpv6h->MsgType == 11) || (pdhcpv6h->MsgType == 3))){// 11 information request(no status), 1 solicit(status)
			return 0;
		}
		Log(LOG_LEVEL_NORMAL, "dhcpv6 request captrue!");

		newskb = dhcp_bufalloc(skb->len + 2, skb->dev, ETH_P_IPV6);
		if (newskb == NULL)
			return -1;
			
		memset(&Dhcpv6msglist, 0, sizeof(Dhcpv6msglist));	
		p = &Dhcpv6msglist;
		MsgLen = payloadlen - sizeof(DHCPV6_HDR) - 1;
		pMsg = &pdhcpv6h->Msg[0];
		while (pos < MsgLen){
			pdhcpmsghdr = (PDHCPV6_MSG_HDR)pMsg;
			SubMsgLen = htons(pdhcpmsghdr->Length);
			OptId = htons(pdhcpmsghdr->option);
			if (OptId == 6){
				pDhcpMsgNode = (PDHCPV6_MSG_NODE)kmalloc(100, GFP_ATOMIC);
				if (pDhcpMsgNode == NULL)
					return -1;
				
				pDhcpMsgNode->dhcpv6msghdr.option = htons(OptId);
				pDhcpMsgNode->dhcpv6msghdr.Length = htons(SubMsgLen + sizeof(unsigned short));
				memcpy(&pDhcpMsgNode->dhcpv6msghdr.Value[0],
					&pdhcpmsghdr->Value[0],
					SubMsgLen);
				pDhcpMsgNode->dhcpv6msghdr.Value[SubMsgLen] = 0x00;
				pDhcpMsgNode->dhcpv6msghdr.Value[SubMsgLen + 1] = 0x40;
				pudph->len = htons(htons(pudph->len) + sizeof(unsigned short));
				piphv6->payload_len = pudph->len; 
				skb->len += sizeof(unsigned short);
			}
			else {
				pDhcpMsgNode = (PDHCPV6_MSG_NODE)kmalloc(100, GFP_ATOMIC);
				if (pDhcpMsgNode == NULL)
					return -1;
				
				pDhcpMsgNode->dhcpv6msghdr.option = htons(OptId);
				pDhcpMsgNode->dhcpv6msghdr.Length = htons(SubMsgLen);
				memcpy(&pDhcpMsgNode->dhcpv6msghdr.Value[0],
					&pdhcpmsghdr->Value[0],
					SubMsgLen);	
			}
			pDhcpMsgNode->next = NULL;
			p->next = pDhcpMsgNode;
			p = pDhcpMsgNode;
			pos += (SubMsgLen + sizeof(DHCPV6_MSG_HDR) - sizeof(unsigned char));
			pMsg += (SubMsgLen + sizeof(DHCPV6_MSG_HDR) - sizeof(unsigned char));
		}

		memcpy(newskb->data, skb->data, skb->len - sizeof(unsigned short));
		newskb->len = skb->len;
		skb_reset_network_header(newskb);

		pdhcpv6h = (PDHCPV6_HDR)(newskb->data + sizeof(struct ipv6hdr) + sizeof(struct udphdr));
		p = Dhcpv6msglist.next;
		pMsg = &pdhcpv6h->Msg[0];
		
		while (p != NULL){
			pDhcpMsgNode = (PDHCPV6_MSG_NODE)p;
			b = p->next;
			memcpy(pMsg,
				&pDhcpMsgNode->dhcpv6msghdr,
				htons(pDhcpMsgNode->dhcpv6msghdr.Length) + sizeof(DHCPV6_MSG_HDR) - 1);
			pMsg += (htons(pDhcpMsgNode->dhcpv6msghdr.Length) + sizeof(DHCPV6_MSG_HDR) - 1);
			kfree(pDhcpMsgNode);
			p = b;
		}
		
		do{
			  struct udphdr *pudp_header = NULL; //UDPÍ·Ö¸Õë
			  PUDP6_PSD_HEADER pudp_psd_header = NULL;
			  unsigned short attachsize = 0; 
			  piphv6 = newskb->data;
			  pudp_header = (struct udphdr *)(newskb->data + sizeof(struct ipv6hdr));
			  attachsize = htons(piphv6->payload_len);
			  pudp_psd_header = kmalloc(1500, GFP_ATOMIC);
			  if (pudp_psd_header == NULL)
			  	return -1;
			  memset(pudp_psd_header, 0, attachsize + sizeof(UDP6_PSD_HEADER));
			  memcpy(pudp_psd_header->destip6, &piphv6->daddr, 16);
			  memcpy(pudp_psd_header->sourceip6, &piphv6->saddr, 16);
			  pudp_psd_header->mbz = 0;
			  pudp_psd_header->ptcl = 0x11;
			  pudp_psd_header->udpl = htons(attachsize);
			  pudp_header->check = 0;
			  memcpy((unsigned char *)pudp_psd_header + sizeof(UDP6_PSD_HEADER),
			   (unsigned char *)pudp_header, attachsize);
			  pudp_header->check = ip_checksum((unsigned short *)pudp_psd_header,
			   attachsize + sizeof(UDP6_PSD_HEADER));
			  kfree(pudp_psd_header);
			  
		}while (0);

		SENDSKB(newskb);
		return 1;
	}
	return 0;
}

int dhcpv6_rx_captrue(struct sk_buff* skb, unsigned char *dnsv6, char *aftraddr)
{
	struct ipv6hdr* piphv6 = NULL;
	struct udphdr *pudph = NULL;
	unsigned int payloadlen = 0, MsgLen = 0, pos = 0, SubMsgLen = 0;
	PDHCPV6_HDR pdhcpv6h = NULL;
	unsigned char *pMsg = NULL;
	PDHCPV6_MSG_HDR pdhcpmsghdr = NULL;
	unsigned int OptId = 0, uiDnsLen = 0, bDns = 0, bAftr = 0;
	
	piphv6 = (struct ipv6hdr*)skb->data;
	if (piphv6->nexthdr != 0x11){
		return -1;
	}
	pudph = (struct udphdr *)(skb->data + sizeof(struct ipv6hdr));
	if ((pudph->source == htons(0x0223)) && (pudph->dest == htons(0x0222))){
		payloadlen = htons(pudph->len) - sizeof(struct udphdr);
		pdhcpv6h = (PDHCPV6_HDR)((unsigned char *)pudph + sizeof(struct udphdr));	
		if (pdhcpv6h->MsgType == 0x07){
			MsgLen = payloadlen - sizeof(DHCPV6_HDR);
			pMsg = &pdhcpv6h->Msg[0];
			while (pos < MsgLen){
				pdhcpmsghdr = (PDHCPV6_MSG_HDR)pMsg;
				SubMsgLen = htons(pdhcpmsghdr->Length);
				OptId = htons(pdhcpmsghdr->option);
				if (OptId == 23) //reply 
				{
					uiDnsLen = htons(pdhcpmsghdr->Length);	
					if (uiDnsLen > 16)
						memcpy(dnsv6, &pdhcpmsghdr->Value[0], 16);
					else
						memcpy(dnsv6, &pdhcpmsghdr->Value[0], uiDnsLen);
					bDns = 1;
				}
				if (OptId == 64){
					memcpy(aftraddr, &pdhcpmsghdr->Value[0], htons(pdhcpmsghdr->Length));
					bAftr = 1;
					do 
					{
						char named[128] = {0};
						unsigned int offset = 0, len = 0;
						
						memset(named, '.', SubMsgLen);
						while (aftraddr[offset] != 0x00)
						{
							len = aftraddr[offset];
							memcpy(&named[offset], &aftraddr[offset + 1], len);
							offset += (len + 1);
						}
						named[offset - 1] = '\0';				
						strcpy(aftraddr, named);
					}while(0);
				}
				pos += (SubMsgLen + sizeof(DHCPV6_MSG_HDR) - sizeof(unsigned char));
				pMsg += (SubMsgLen + sizeof(DHCPV6_MSG_HDR) - sizeof(unsigned char));
			}
		}
	}
	return 1;
}

