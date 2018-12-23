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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/in6.h>
#include <linux/ipv6.h>
#include <linux/icmp.h>
#include <net/ipv6.h>
#include <net/udp.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <net/tcp.h>
#include <linux/string.h>
#include <linux/if_ether.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/rtc.h>
#include <linux/inet.h>
#include <linux/netlink.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <net/net_namespace.h>
#include <net/sock.h>
#include <asm/byteorder.h>
#include "bplus.h"
#include "packet.h"
#include "storage.h"
#include "global.h"
#include "memdebug.h"
#include "log.h"
#include "help.h"
#include "nlmessage.h"
#include "netlink.h"
#include "defrag6.h"
#include "mac.h"
#include "tccorestat.h"

static int IPv4LenCheck(struct sk_buff* skb)
{
	struct iphdr* iph = NULL;
	unsigned int len = 0;
	unsigned char *pmsg = NULL;
	
#define PAYLOADLEN 28
	iph = ip_hdr(skb);
	if(!iph)
	{   
		Log(LOG_LEVEL_ERROR, "iph error");
		return -1; 
	}  
	
	if ((iph->frag_off & htons(IP_DF)) && 
		skb->len + sizeof(struct ipv6hdr) > GlobalCtx.usMtu)
	{	
		Log(LOG_LEVEL_WARN, "DF is set and len more than %d", GlobalCtx.usMtu);
		pmsg = ConstructIcmpErrMsg(skb, &len);
		if (pmsg)
		{
			send_async_nlmsg(skb->data, len);	
		}
	
		return -1;	
	}
	
	return 1;
}
/** 
* @fn static unsigned short getstartport(unsigned short Port)
* @brief Through a port calculate start port
* 
* @param[in] Port
* @retval Start port
*/
static unsigned short getstartport(unsigned short Port)
{
	Port = ((Port - START_PORT) / GlobalCtx.usPortRange) * GlobalCtx.usPortRange + START_PORT;

	return Port;
}

/** 
* @fn fragsk_bufalloc
* @brief allocate a sk buffer
* 
* @param[in] length packet length
* @param[in] dev adapter device
* @param[in] usProtocol packet protocol
* @retval skb_buff pointer 
*/
static struct sk_buff * fragsk_bufalloc(unsigned int length,
	struct net_device *dev,
	char *mac,
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
	memcpy(eth_t->h_source, mac, 6);
	
    new_skb_buf->pkt_type = PACKET_HOST;
    new_skb_buf->protocol = htons(usProtocol);
    new_skb_buf->dev = dev;
	
	skb_reset_mac_header(new_skb_buf);
    skb_pull(new_skb_buf, ETH_HLEN);
		
	return new_skb_buf;
}

/** 
* @fn int CreateTunnel(PTUNNEL_CTX t)
* @brief Create tunnel node
* 
* @param[in] t Tunnel Context
* @retval -1 Failure
* @retval  1 Success
*/
int CreateTunnel(PTUNNEL_CTX t)
{	
	KEY_V4 keyv4;
	KEY_V6 keyv6;
	int ret = 0;
	PTUNNEL_CTX pTunnel = NULL;
	
	keyv4.usPort = t->usStartPort;
	keyv4.uiExtip = t->uiExtIP;
	
	pTunnel = (PTUNNEL_CTX)FindElement(GlobalCtx.pStroageCtx, (PKEY)&keyv4, sizeof(KEY_V4), ELEMENT_4);
	if (pTunnel != NULL)
	{
		return -1;
	}
	spin_lock(&GlobalCtx.StorageLock);
	ret = InsertElement(GlobalCtx.pStroageCtx,
		(PKEY)&keyv4,
		sizeof(KEY_V4),
		t, ELEMENT_4);
	if (ret < 0)
	{
		spin_unlock(&GlobalCtx.StorageLock);
		return -1;
	}
	else
		spin_unlock(&GlobalCtx.StorageLock);
	
	spin_lock(&GlobalCtx.StorageLock);
	memcpy(keyv6.ucCltIPv6, t->ucCltIPv6, 16);
	ret = InsertElement(GlobalCtx.pStroageCtx,
		(PKEY)&keyv6,
		sizeof(KEY_V6),
		t, ELEMENT_6);
	if (ret < 0)
	{
		DeleteElement(GlobalCtx.pStroageCtx, (PKEY)&keyv4, sizeof(KEY_V4), ELEMENT_4);
		spin_unlock(&GlobalCtx.StorageLock);
		return -1;
	}
	else
		spin_unlock(&GlobalCtx.StorageLock);
	return 1;
}

/** 
* @fn int DeleteTunnel(unsigned char *ucCltIPv6Addr, unsigned short *Port, unsigned int ExtIP)
* @brief Delete tunnel node
* 
* @param[in] ucCltIPv6Addr Client IPv6 address
* @param[out] Port Start Port
* @param[out] ExtIP Extern IPv4 address
* @retval -1 Failure
* @retval  1 Success
*/
int DeleteTunnel(unsigned char *ucCltIPv6Addr, unsigned short *Port, unsigned int *ExtIP)
{
	KEY_V6 key6;
	KEY_V4 key4;
	PTUNNEL_CTX t = NULL;
	char addr[128] = {0};
	
	memcpy(key6.ucCltIPv6, ucCltIPv6Addr, 16);
	
	Log(LOG_LEVEL_NORMAL, "Client :%s", apr_inet_ntop(AF_INET6, ucCltIPv6Addr, addr, sizeof(addr)));

	spin_lock(&GlobalCtx.StorageLock);
	t = LookupTunnel_v6((PKEY_V6)&key6);
	if (t != NULL)
	{	
		if (DeleteElement(GlobalCtx.pStroageCtx,
					&key6,
					sizeof(key6), 
					ELEMENT_6) == NULL)
		{
			Log(LOG_LEVEL_ERROR, "DeleteElement 6 error!");
			
		}
		key4.usPort = *Port = t->usStartPort;
		key4.uiExtip = *ExtIP = t->uiExtIP;
		Log(LOG_LEVEL_NORMAL, "Port = %d, ExIP = %x", key4.usPort, key4.uiExtip);
		if (DeleteElement(GlobalCtx.pStroageCtx,
					&key4,
					sizeof(key4), 
					ELEMENT_4) == NULL)
		{
			Log(LOG_LEVEL_ERROR, "DeleteElement 4 error!");
			
		}

		dbg_free_mem(t);
		spin_unlock(&GlobalCtx.StorageLock);

		return 1;
	}
	else
		spin_unlock(&GlobalCtx.StorageLock);

	
	return -1;
}

/** 
* @fn int LookupTunnel_v6(PKEY_V6 keyv6)
* @brief Find whether the user is effective
* 
* @param[in] IPv6Address IPv6 address
* @retval  0 Not found
* @retval NULL Failure
* @retval PTUNNEL_CTX Success
*/
PTUNNEL_CTX LookupTunnel_v6(PKEY_V6 pkeyv6)
{	
	PTUNNEL_CTX pTunnel = NULL;
	
	pTunnel = (PTUNNEL_CTX)FindElement(GlobalCtx.pStroageCtx, (PKEY)pkeyv6, 16, ELEMENT_6);
	if (pTunnel == NULL)
		return NULL;
	
	return pTunnel;
}

/** 
* @fn int LookupTunnel_v4(PKEY_V4 pkeyv4, char *ipv6address)
* @brief Find whether the user is effective
* 
* @param[in] addr IPv4 address
* @param[in] usport begin port 
* @retval  NULL 		Failure
* @retval  PTUNNEL_CTX  Success
*/
PTUNNEL_CTX LookupTunnel_v4(PKEY_V4 pkeyv4)
{	
	PTUNNEL_CTX pTunnel = NULL;
	
	pTunnel = (PTUNNEL_CTX)FindElement(GlobalCtx.pStroageCtx, (PKEY)pkeyv4, sizeof(KEY_V4), ELEMENT_4);
	if (pTunnel == NULL)
		return NULL;
	
	return pTunnel;
}


/** 
* @fn static void FetchIPAndPort_skbuf(struct sk_buff* skb,
	unsigned int *uiaddr,
	unsigned short *usport)
* @brief Fetch  IPv4 and port from sk_buf 
* @param[in] 	skb Packet
* @param[out] 	uiaddr IPv4 address
* @param[out] 	usport Port 
* @retval  void 
*/
static void FetchIPAndPort_skbuf(struct sk_buff* skb,
	unsigned int *uiaddr,
	unsigned short *usport)
{
	struct iphdr *piphdr = NULL;
	struct tcphdr *ptcphdr = NULL;
	struct udphdr *pudphdr = NULL;
	struct icmphdr *picmphdr = NULL;
	
	piphdr = ip_hdr(skb);
	*uiaddr = htonl(piphdr->daddr);
	switch (piphdr->protocol)
	{
	case IPPROTO_TCP:
		{
			ptcphdr = (struct tcphdr *)(skb->data + sizeof(struct iphdr));
			*usport = htons(ptcphdr->dest);
		}
		break;
	case IPPROTO_UDP:
		{
			pudphdr = (struct udphdr *)(skb->data + sizeof(struct iphdr));
			*usport = htons(pudphdr->dest);
		}
		break;
	case IPPROTO_ICMP:
		{
			picmphdr = (struct icmphdr *)(skb->data + sizeof(struct iphdr));
			if (FilterIcmpErrOuter(picmphdr, skb->len- sizeof(struct iphdr)) > 0)
			{
				//ICMP ERROR PROCESS
				if (OuterICMPErr_get(picmphdr, usport, uiaddr) > 0)
				{
					Log(LOG_LEVEL_WARN, "OuterICMPErr_get:%x,%d",*uiaddr, *usport);
					return;
				}	
			}						
			*usport = htons(picmphdr->un.echo.id);
		}
		break;
	default :
		break;
	}
}

/** 
* @fn static int HandleIPinIP_skbuf(struct sk_buff* skb)
* @brief Handle IPv4 over IPv6 Packet
* 
* @param[in] 	skb Packet
* @param[out] 	IPv6Address source IPv6 address
* @retval  void 
*/
static void FetchIPv6_skbuf(struct sk_buff* skb, unsigned char *IPv6Address)
{
	struct ipv6hdr *pipv6hdr = NULL;
	pipv6hdr = ipv6_hdr(skb);
	
	memcpy(IPv6Address, pipv6hdr->saddr.in6_u.u6_addr8, 16);
}

/** 
* @fn int HandleIPinIP_skbuf(struct sk_buff* skb)
* @brief Handle IPv4 over IPv6 Packet
* 
* @param[in] skb Packet
* 
* @retval void 
*/
void HandleIPinIP_skbuf(struct sk_buff* skb)
{
	struct ethhdr* eth_t = NULL;
	struct ipv6hdr *pipv6hdr = NULL;
	unsigned int uiPayLoad = 0;
	
	eth_t = eth_hdr(skb);
	eth_t->h_proto = htons(ETH_P_IP);

	pipv6hdr = ipv6_hdr(skb);
	uiPayLoad = htons(pipv6hdr->payload_len);
	
	memmove((void *)pipv6hdr,
		(void *)((unsigned char *)pipv6hdr + sizeof(struct ipv6hdr)),
		uiPayLoad);

	//move  pointer of tail
	skb->protocol = htons(ETH_P_IP);
	skb->pkt_type = PACKET_HOST;
	skb->tail -= sizeof(struct ipv6hdr);
	skb->len -= sizeof(struct ipv6hdr);
	
	skb_reset_transport_header(skb);
	skb_set_transport_header(skb, sizeof(struct iphdr));
}

/** 
* @fn  int HandleIPv6_skbuf(struct sk_buff* skb)
* @brief Handle IPv6 Packet
* 
* @param[in] skb Packet
* 
* @retval  1 		Success 
* @retval -1		Failure 
* @retval  0		Normal
*/
int HandleIPv6_skbuf(struct sk_buff* skb)
{
	struct ipv6hdr* piph_v6 = NULL;
	struct iphdr *piphdr = NULL;

	KEY_V6 keyv6;
	PTUNNEL_CTX t = NULL;
	char s[128] = {0};
	
	piph_v6 = ipv6_hdr(skb);
	if (piph_v6 == NULL)
		return -1;

	switch(piph_v6->nexthdr)
	{
	case IPINIP:
		{
			FetchIPv6_skbuf(skb, keyv6.ucCltIPv6);
			//Lookup user vaild
			spin_lock(&GlobalCtx.StorageLock);
			t = LookupTunnel_v6(&keyv6);
			if (t != NULL)
			{	
				spin_unlock(&GlobalCtx.StorageLock);
				Log(LOG_LEVEL_NORMAL,
					"Lookup the IPv6 Address %s in Tunnel.",
					apr_inet_ntop(AF_INET6, keyv6.ucCltIPv6, s, sizeof(s)));
				HandleIPinIP_skbuf(skb);
#if TC_STAT_DEF 								
				SetTCCoreStatInfo(TX_BYTE, PACKETLEN(skb));
				SetTCCoreStatInfo(TX_PACKET, 1);
#endif
#if 1
				patch_tcpmss(skb->data, skb->len);
#endif
				SendSkbuf(skb, ETH_P_IP);
			}
			else
			{
				spin_unlock(&GlobalCtx.StorageLock);
				piphdr = (struct iphdr *)(skb->data + sizeof(struct ipv6hdr));
				//crash ip packet
				if (IsCrashIPinPool(htonl(piphdr->saddr)))
				{	
					unsigned char *pcrashmsg = NULL;
					unsigned int uiLen = 0;
					Log(LOG_LEVEL_NORMAL, "Receive crash packet");
					pcrashmsg = ConstructCrashPktMsg(skb, &uiLen);
					if (pcrashmsg)
					{
						send_async_nlmsg(pcrashmsg, uiLen);
						dbg_free_mem(pcrashmsg);
					}
				}
			}
			return -1;
		}
		break;
	case NEXTHDR_FRAGMENT:
		{	
			unsigned short offset = 0;
			struct sk_buff *newskb = NULL;
			unsigned char mf = 0;
			unsigned int length = 0;
			PFRAG_CB pfragcb = NULL;
			struct _tag_frag_head * fraghead = NULL;
			
			if (IsFragIPinIP(skb) == 0)
				return 1;

			//Fetch source IPv6 address
			FetchIPv6_skbuf(skb, keyv6.ucCltIPv6);
			//Lookup user vaild
			spin_lock(&GlobalCtx.StorageLock);
			t = LookupTunnel_v6(&keyv6);
			spin_unlock(&GlobalCtx.StorageLock);
			if (t != NULL)
			{
				Log(LOG_LEVEL_NORMAL, 
					"Lookup the IPv6 Address %s in Tunnel.", 
					apr_inet_ntop(AF_INET6, keyv6.ucCltIPv6, s, sizeof(s)));
				offset = GetFragOffset(skb, &mf, &length);
				newskb = fragsk_bufcopy(skb);
				if (newskb == NULL)
					return -1;
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
					Log(LOG_LEVEL_NORMAL, "the packet is completed!");
					fragprocess(fraghead);
				}
				fraghash_unlock();
			}
	
			return -1;
		}
		break;
	default :
		//other IPv6 Packet
		return 0;
		break;
	}

	return 1;
}

/** 
* @fn  int HandleIPv4_skbuf(struct sk_buff* skb)
* @brief Handle IPv4 Packet
* 
* @param[in] skb Packet
* 
* @retval  1 		Success 
* @retval -1		Failure 
* @retval  0		Normal
*/
int HandleIPv4_skbuf(struct sk_buff* skb)
{	
	struct iphdr *piphdr = NULL;
	unsigned char dstipv6[16] = {0};
	KEY_V4 keyv4;
	PTUNNEL_CTX t = NULL;
	char s[128] = {0};
	char w[128] = {0};
	
	piphdr = ip_hdr(skb);
	if (piphdr == NULL)
		return -1;
#if 1
	if (IPv4LenCheck(skb) < 0)
	{
		return -1;
	}
#endif
	FetchIPAndPort_skbuf(skb, &keyv4.uiExtip, &keyv4.usPort);
	//Calculate start port
	keyv4.usPort = getstartport(keyv4.usPort);
	spin_lock(&GlobalCtx.StorageLock);
	t = LookupTunnel_v4(&keyv4);
	if (t != NULL)
	{	
		memcpy(dstipv6, t->ucCltIPv6, 16);
		spin_unlock(&GlobalCtx.StorageLock);
		keyv4.uiExtip = htonl(keyv4.uiExtip);
		Log(LOG_LEVEL_NORMAL, 
					"Lookup IPv4:%s,Port:%d,IPv6:%s in Tunnel",
					apr_inet_ntop(AF_INET, &keyv4.uiExtip, s, sizeof(s)),
					keyv4.usPort, 
					apr_inet_ntop(AF_INET6, dstipv6, w, sizeof(w)));
#if 1	
		patch_tcpmss(skb->data, skb->len);
#endif	
		if (skb_is_nonlinear(skb) > 0)
		{
			Log(LOG_LEVEL_NORMAL, "IPv4 Len :%d fragment packet", skb->len);
			//fragment packet 
			return IPv4FragandIPinIPExtend(skb, dstipv6);
		}
		else if ((skb->len + sizeof(struct ipv6hdr) > GlobalCtx.usMtu) &&
			(skb_is_nonlinear(skb) == 0))
		{
			Log(LOG_LEVEL_NORMAL, "IPv4 Len %d more than Mtu Len %d packet", skb->len, GlobalCtx.usMtu);

			return IPv4FragandIPinIP(skb, dstipv6);
		}
		else
		{
			Log(LOG_LEVEL_NORMAL, "IPv4 normal packet %d", skb->len);
			return CreateIPinIP_skbuf(skb, dstipv6);
		}
		
	}
	else
		{
			spin_unlock(&GlobalCtx.StorageLock);
#if 0
			keyv4.uiExtip = htonl(keyv4.uiExtip);
			Log(LOG_LEVEL_WARN,
				"not found ipv4:%s",
				apr_inet_ntop(AF_INET, &keyv4.uiExtip, s, sizeof(s)));
#endif
		}
	return 1;
}

/** 
* @fn  uint Handle_skbuf(struct sk_buff* skb)
* @brief Filter IPv4 or IPv4 over IPv6 Packet
* 
* @param[in] skb Packet
* 
* @retval  1 		Success 
* @retval -1		Failure 
* @retval  0		Normal 
*/
int Handle_skbuf(struct sk_buff* skb)
{
	struct ethhdr* eth;
	
	eth = (struct ethhdr* )eth_hdr(skb);
	if (eth == NULL)
		return -1;

	if (IPFilter(skb, htons(eth->h_proto)) < 0)
	{
		return 0;
	}
	switch (htons(eth->h_proto))
	{
	//IPv6 
	case ETH_P_IPV6:
		{
			return HandleIPv6_skbuf(skb);
		}
		break;
	//IPv4 
	case ETH_P_IP:
		{
			return HandleIPv4_skbuf(skb);
		}
		break;
	default:
		break;
	}
	
	return 1;	
}


/**
* @fn int SendSkbuf(struct sk_buff *skb)
* @brief Send the new_skb
*
* @param[in] new_skb 
*
* @retval  1 Success
* @retval -1 Failure
*/
int SendSkbuf(struct sk_buff *skb, unsigned short protocol)
{
	struct sk_buff * new_skb_buf = NULL;
	struct ethhdr* eth_t = NULL;
	
	new_skb_buf = dev_alloc_skb(skb->len + ETH_HLEN +  5);
    if(!new_skb_buf)
        return -1;

    skb_reserve(new_skb_buf, 2);
    skb_put(new_skb_buf, ETH_HLEN + skb->len);

	eth_t = (struct ethhdr*)new_skb_buf->data;
	eth_t->h_proto = htons(protocol);
	
    new_skb_buf->pkt_type = PACKET_HOST;
    new_skb_buf->protocol = skb->protocol;
    new_skb_buf->dev = skb->dev;
	
	skb_reset_mac_header(new_skb_buf);
    skb_pull(new_skb_buf, ETH_HLEN);

	//Set MAC
	eth_t = (struct ethhdr*)eth_hdr(skb);
	SkbufMacSet(new_skb_buf, eth_t->h_dest);
	
	memcpy(new_skb_buf->data, skb->data, skb->len);

	skb_reset_network_header(new_skb_buf);	
	
    SENDSKB(new_skb_buf);

	return 1;
}



/**
* @fn int CreateIPinIP_skbuf(struct sk_buff *skb, unsigned char *ipv6address)
* @brief Create IPv4 over IPv6 Packet and Send skbuff
*
* @param[in] skb Original skbuf
* @param[in] ipv6address client ipv6 address
*
* @retval  1 Success
* @retval -1 Failure
* @retval  0 Normal
*/
int CreateIPinIP_skbuf(struct sk_buff *skb, unsigned char *ipv6address)
{
	//check length
	struct ethhdr* eth_t = NULL;
	struct ipv6hdr *ipv6hdr_t = NULL;
	struct sk_buff *newskbbuff = NULL;
	unsigned short usPayload = 0;

	usPayload = skb->len;
	//Set MAC
	eth_t = (struct ethhdr*)eth_hdr(skb);
	
	newskbbuff = fragsk_bufalloc(usPayload + sizeof(struct ipv6hdr),
		skb->dev,
		eth_t->h_dest,
		ETH_P_IPV6);
	if (newskbbuff == NULL)
		return -1;
	
	ipv6hdr_t = (struct ipv6hdr *)(newskbbuff->data);
	ipv6hdr_t->version = 0x06;
	ipv6hdr_t->priority = 0x00;
	ipv6hdr_t->flow_lbl[0] = 0;
	ipv6hdr_t->flow_lbl[1] = 0;
	ipv6hdr_t->flow_lbl[2] = 0;
	ipv6hdr_t->hop_limit = 0x40;
	ipv6hdr_t->nexthdr = IPINIP;
	ipv6hdr_t->payload_len = htons(usPayload);
	memcpy(&ipv6hdr_t->daddr, ipv6address, 16);
	memcpy(&ipv6hdr_t->saddr, GlobalCtx.ucTcIPv6, 16);
	
	memcpy(newskbbuff->data + sizeof(struct ipv6hdr), skb->data, skb->len);

	skb_reset_network_header(newskbbuff);
	
#if TC_STAT_DEF 				
	SetTCCoreStatInfo(RX_BYTE, PACKETLEN(newskbbuff));
	SetTCCoreStatInfo(RX_PACKET, 1);
#endif

	SENDSKB(newskbbuff);
	
	return 1;
}

/**
* @fn static int TunnelTimeCheck(struct BPTree*ptree4,
	struct BPTree*ptree6,
	PELEMENT pElement)
* @brief Check the overtime node
*
* @param[in] ptree4
* @param[in] ptree6
*
* @retval  1 none
* @retval -1 the node overtime
*/
static int TunnelTimeCheck(struct BPTree*ptree4,
	struct BPTree*ptree6,
	PELEMENT pElement) 
{
	PTUNNEL_CTX pTunnelCtx = (PTUNNEL_CTX)pElement;
	KEY_V4 key4;
	KEY_V6 key6;
	char s[128] = {0};
	char w[128] = {0};
	char sndmsg[128] = {0};
	
	if (pElement == NULL)
	{
		Log(LOG_LEVEL_ERROR, "pElement is null");
		return 1;
	}
	
	pTunnelCtx->ulLiveTime -= GlobalCtx.uiMaxLifeTime / 4;
	if (pTunnelCtx->ulLiveTime <= 0)
	{
		key4.uiExtip = pTunnelCtx->uiExtIP;
		key4.usPort = pTunnelCtx->usStartPort;
		memcpy(key6.ucCltIPv6, pTunnelCtx->ucCltIPv6, 16);
		Log(LOG_LEVEL_ERROR, 
			"Delete IPv4:%s,Port:%d,IPv6:%s in Tunnel, Size :%d",
			apr_inet_ntop(AF_INET, &key4.uiExtip, s, sizeof(s)),
			key4.usPort, 
			apr_inet_ntop(AF_INET6, key6.ucCltIPv6, w, sizeof(w)), 
			SizeOfElement(GlobalCtx.pStroageCtx));
		
		if (bpdelete(ptree4, (char *)&key4, sizeof(key4)) == NULL)
		{
			Log(LOG_LEVEL_ERROR, "bpdelete 4 error");
		}
		if (bpdelete(ptree6, (char *)&key6, sizeof(key6)) == NULL)
		{
			Log(LOG_LEVEL_ERROR, "bpdelete 6 error");
		}
		
		Log(LOG_LEVEL_NORMAL, 
			"delete node size %d",
			SizeOfElement(GlobalCtx.pStroageCtx));
#if TC_STAT_DEF 						
		DelTCCoreStatInfo(USER_SIZE, 1);
#endif
		//Notice the application layer destroyed address pool
		ConstructTunMsg(sndmsg,
			key6.ucCltIPv6, 
			NETLINK_DELETE_NODE,
			key4.uiExtip,
			key4.usPort, 
			0);
		send_async_nlmsg(sndmsg, sizeof(sndmsg));
		
		dbg_free_mem(pTunnelCtx);
				
		return -1;
	}
	return 1;
}

/**
* @fn void TunnelTimeOut(unsigned long data)
* @brief Traverse tunnel node 
*
* @param[in] ptree4
* @param[in] ptree6
*
* @retval  void
*/
void TunnelTimeOut(unsigned long data)
{
	Log(LOG_LEVEL_NORMAL, "%d S time out...", GlobalCtx.uiMaxLifeTime/4);
	
	bptraverse(&GlobalCtx.pStroageCtx->BPTreeCtx[ELEMENT_4],
		&GlobalCtx.pStroageCtx->BPTreeCtx[ELEMENT_6], 
		TunnelTimeCheck);

	GlobalCtx.StorageTimer.expires = jiffies + (GlobalCtx.uiMaxLifeTime/4) * HZ;
 	add_timer(&GlobalCtx.StorageTimer);
}


/*  struct frag_hdr {
          __u8    nexthdr;
          __u8    reserved;
         __be16  frag_off;
         __be32  identification;
 };*/
/*
* @fn  int IPv4FragandIPinIP(struct sk_buff* skb, 
						unsigned char *ucDestIPv6)
* @brief IPV6 will add to the head of the packet length is 
*	more than MTU IPV4 packets fragment
* 
* @param[in] IPv4 packet length more than MTU
* @param[in] ucDestIPv6	Client IPv6 address
* 
* @retval  1 		Success 
* @retval -1		Failure 
*/
int IPv4FragandIPinIP(struct sk_buff* skb, 
						unsigned char *ucDestIPv6)
{
	unsigned int off = 0, flen = 0, mtu = 0;
	unsigned int fmtu = 0, nfrag = 0, flen0 = 0, fleno = 0, fcc = 0;

	struct iphdr *piphdr = NULL;
	unsigned int len = 0;
	struct sk_buff *sdbuf = NULL;
	struct ipv6hdr *pipv6hdr = NULL;
	struct frag_hdr *pfraghdr = NULL;
	unsigned int uiFragHdrSize = 0;
	unsigned int uiId = 0;
	struct ethhdr* eth;
	
	eth = (struct ethhdr* )eth_hdr(skb);
	uiFragHdrSize = sizeof(struct ipv6hdr) + sizeof(struct frag_hdr);
	piphdr = (struct iphdr *)ip_hdr(skb);
	uiId = piphdr->id;
		
	mtu = GlobalCtx.usMtu;
	len = skb->len;
	
	flen0 = 0;		/* XXX: silence compiler */
	fleno = 0;

	fmtu = (mtu - uiFragHdrSize) & ~7;
	nfrag = len / fmtu;			/* always >= 1 */
			
	if (len % fmtu != 0)
		nfrag++;

	/* len = fmtu * (nfrag - 1) + rem; 0 <= rem < fmtu */
	fleno = (len / nfrag) & ~7;		/* lower medium size */
	flen0 = len - fleno * (nfrag - 1);	/* remainder */
	flen0 = (flen0 + 7) & ~7;
	if (flen0 > fmtu) {
		/* too much remainder, switch to bigger medium size,
		   but still <= fmtu */
		fleno += 8;
		/* recompute remainder (shall be this time <= fmtu) */
		flen0 = len - (fleno * (nfrag - 1));
		flen0 = (flen0 + 7) & ~7;
	}
	/* biggest should be first, smallest last */
	if (flen0 < fleno)
		flen0 = fleno;

	for (off = 0; off < len; off += flen) 
	{
		flen = len - off;
		if (flen > mtu - uiFragHdrSize) 
		{
			if (off == 0) /* first fragment */
				flen = flen0;
			else /* intermediate fragment */
				flen = fleno;
		}
		sdbuf = fragsk_bufalloc(flen + uiFragHdrSize,
			skb->dev,
			eth->h_dest,
			ETH_P_IPV6);
		if (sdbuf == NULL)
			return -1;
		
		pipv6hdr = (struct ipv6hdr *)sdbuf->data;
		pipv6hdr->version = 0x06;
		pipv6hdr->priority = 0x00;
		pipv6hdr->nexthdr = 0x2c;
		pipv6hdr->hop_limit = 64;
		pipv6hdr->flow_lbl[0] = 0;
		pipv6hdr->flow_lbl[1] = 0;
		pipv6hdr->flow_lbl[2] = 0;
		pipv6hdr->payload_len = htons(flen + sizeof(struct frag_hdr));
		memcpy(&pipv6hdr->saddr, GlobalCtx.ucTcIPv6, 16);
		memcpy(&pipv6hdr->daddr, ucDestIPv6, 16);
		pfraghdr = (struct frag_hdr *)(sdbuf->data + sizeof(struct ipv6hdr));
		pfraghdr->nexthdr = IPINIP;
		pfraghdr->identification = htonl(uiId);
		pfraghdr->reserved = 0;
		pfraghdr->frag_off = htons(off);
		if (flen + off < len)
			pfraghdr->frag_off = htons(off | 0x01);
		
		memcpy(sdbuf->data + uiFragHdrSize, skb->data + off, flen);
		fcc = flen + uiFragHdrSize;
			
		skb_reset_transport_header(sdbuf);
		skb_set_transport_header(sdbuf, sizeof(struct ipv6hdr));

		skb_reset_network_header(sdbuf);
		
#if TC_STAT_DEF 				
		SetTCCoreStatInfo(RX_BYTE, PACKETLEN(sdbuf));
		SetTCCoreStatInfo(RX_PACKET, 1);
#endif

		SENDSKB(sdbuf);
	}
	
	return 1;
}


/*
* @fn  int IPv4FragandIPinIPExtend(struct sk_buff* skb, 
						unsigned char *ucDestIPv6)
* @brief IPV6 will add to the head of the packet length is 
*	more than MTU IPV4 packets fragment
* 
* @param[in] IPv4 packet length more than MTU
* @param[in] ucDestIPv6	Client IPv6 address
* 
* @retval  1 		Success 
* @retval -1		Failure 
*/
int IPv4FragandIPinIPExtend(struct sk_buff* skb, 
						unsigned char *ucDestIPv6)
{

	if (__skb_linearize(skb) != 0)
	{
		Log(LOG_LEVEL_ERROR, "low memory....\n");
		return -1;
	}


	return IPv4FragandIPinIP(skb, ucDestIPv6);	

}
/*
* @fn unsigned char IsFragIPinIP(struct sk_buff* skb)
* @brief judgment IPv4 over IPv6 packet
* 
* @param[in] skb 
* 
* @retval  1 		yes
* @retval  0		no 
*/
unsigned char IsFragIPinIP(struct sk_buff* skb)
{
	struct ipv6hdr* iph_v6 = NULL;
	struct frag_hdr *fragv6hdr = NULL;
	
	iph_v6 = ipv6_hdr(skb);
	if (iph_v6 == NULL)
		return 0;

	fragv6hdr = (struct frag_hdr *)((unsigned char *)iph_v6 + sizeof(struct ipv6hdr));

	if (fragv6hdr->nexthdr == 0x04)
		return 1;
	
	return 0;
}

/*
* @fn unsigned short GetFragOffset(struct sk_buff* skb,
	unsigned char *mf,
	unsigned short *length)
* @brief Get fragment packet offset value
* 
* @param[in] skb 
* @param[out] mf 	more fragment
* @param[out] length  payload length
* 
* @retval  offset 		
*/
unsigned short GetFragOffset(struct sk_buff* skb,
	unsigned char *mf,
	unsigned short *length)
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

/*
struct iphdr {
 #if defined(__LITTLE_ENDIAN_BITFIELD)
          __u8    ihl:4,
                  version:4;
  #elif defined (__BIG_ENDIAN_BITFIELD)
          __u8    version:4,
                  ihl:4;
  #else
  #error  "Please fix <asm/byteorder.h>"
  #endif
          __u8    tos;
          __be16  tot_len;
          __be16  id;
          __be16  frag_off;
          __u8    ttl;
         __u8    protocol;
         __sum16 check;
         __be32  saddr;
         __be32  daddr;
         
};*/

/*
* @fnint IPFilter(struct sk_buff* skb, unsigned short protocol)
* @brief Filter invaild packet
* 
* @param[in] skb 
* @param[in] protocol
* 
* @retval  1 Accept
* @retval  -1 Drop
*/
int IPFilter(struct sk_buff* skb, unsigned short protocol)
{
	struct iphdr* iph = NULL;
	struct ipv6hdr *iphv6 = NULL;

	switch(protocol)
	{
	case ETH_P_IPV6:
		{
			iphv6 = ipv6_hdr(skb);
			if (memcmp(iphv6->saddr.in6_u.u6_addr8, GlobalCtx.ucTcIPv6, 16) == 0)
			{
				return -1;
			}
		}
		return 1;
	break;
	case ETH_P_IP:
	{
		iph = ip_hdr(skb);

		if ((htonl(iph->daddr) >= GlobalCtx.uiExtBeginIP) &&
			(htonl(iph->daddr) <= GlobalCtx.uiExtEndIP))
		{
			return 1;
		}
		if ((htonl(iph->saddr) >= GlobalCtx.uiExtBeginIP) &&
								(htonl(iph->saddr) <= GlobalCtx.uiExtEndIP))
		{
			return -1;
		}
	}
	break;
	default:
		break;
	}
	
	return -1;
}


void
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

	Log(LOG_LEVEL_NORMAL, "..........mss:%d", mss);
	/* no patch needed */
	if ((mss + IPHDRLEN + TCPHDRLEN) <= (GlobalCtx.usMtu - IP6HDRLEN))
		return;
	
	fix_msscksum(buf4 + TCPCKSUMH, buf4 + i,
		     GlobalCtx.usMtu - (IP6HDRLEN + IPHDRLEN + TCPHDRLEN));
}


/*
* @fn int IsCrashIPinPool(unsigned int extip)
* @brief Judge whether IP address in the address pool
* 
* @param[in] extip  extern ip address
* 
* @retval  1 Accept
* @retval  -1 Drop
*/
int IsCrashIPinPool(unsigned int extip)
{
	if (extip >= GlobalCtx.uiOtherExtBeginIP && 
		extip <= GlobalCtx.uiOtherExtEndIP)
		return 1;

	return 0;
}

/*  struct icmphdr {
   __u8          type;
   __u8          code;
   __sum16       checksum;
   union {
          struct {
                 __be16  id;
                  __be16  sequence;
         } echo;
         __be32  gateway;
         struct {
                 __be16  __unused;
                 __be16  mtu;
         } frag;
    } un;
 };
*/

void SkbufMacSet(struct sk_buff* skb, char *mac)
{
	struct ethhdr* eth = NULL;
	
	eth = (struct ethhdr* )eth_hdr(skb);
	memcpy(eth->h_source, mac, ETH_ALEN);
}


int OuterICMPErr_get(struct icmphdr *picmphdr,
	unsigned short *port,
	unsigned int *extip)
{
	struct iphdr *piphdr = NULL;
	struct tcphdr *ptcphdr = NULL;
	struct udphdr *pudphdr = NULL;

	
	if ((picmphdr->type == ICMP_ECHOREPLY) || (picmphdr->type == ICMP_ECHO))
	 	return -1;
	
	piphdr = (struct iphdr *)((unsigned char *)picmphdr + sizeof(struct icmphdr));
	*extip = htonl(piphdr->saddr);
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

void Dev_queue_xmit(struct sk_buff *skb)
{
	unsigned short protocol = ntohs(skb->protocol);
	unsigned char *dest_mac = NULL;
	int ret = 0;
	//Log(LOG_LEVEL_WARN, "Dev_queue_xmit In------->>>");
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
	ret = dev_queue_xmit(skb);
	if (ret < 0)
	{
		Log(LOG_LEVEL_ERROR, "dev_queue_xmit error");
	}
	//Log(LOG_LEVEL_WARN, "Dev_queue_xmit Out<<<--------------");
}


int FilterIcmpErrOuter(struct icmphdr *picmphdr, unsigned int len)
{
#define FRAG_MORE 0x2000
#define FRAG_MASK 0x001f

#define ICMP_ERR_UNREACHABLE 			3 
#define ICMP_ERR_TIME_EXCEEDED			11
#define ICMP_ERR_BADPARAMETER			12

	int iRet = -1;
	struct iphdr *piphdr = NULL;

	switch (picmphdr->type)
	{
	case ICMP_ERR_UNREACHABLE:
		Log(LOG_LEVEL_WARN, "icmp type:ICMP_ERR_UNREACHABLE");
		break;
	case ICMP_ERR_TIME_EXCEEDED:
		Log(LOG_LEVEL_WARN, "icmp type:ICMP_ERR_TIME_EXCEEDED");
		break;
	case ICMP_ERR_BADPARAMETER:
		Log(LOG_LEVEL_WARN, "icmp type:ICMP_ERR_BADPARAMETER");
		break;
	default :
		return iRet;
		break;
	}
	
	piphdr = (struct iphdr *)((unsigned char *)picmphdr + sizeof(struct icmphdr));
	//Unhandle embed none ipv4 packet
	if (piphdr->ihl < 5 || piphdr->version != 4)
	{
		Log(LOG_LEVEL_WARN, "header length and version error");
        return iRet;
	}
	//check embed ip header checksum
	if (in_cksum((unsigned char *)piphdr, sizeof(struct iphdr)) != 0)
	{
		Log(LOG_LEVEL_WARN, "embed ip header checksum error");
		return iRet;
	}
	//Unhandle embed fragment packet
	if (htons(piphdr->frag_off) & FRAG_MORE || htons(piphdr->frag_off) & FRAG_MASK)
	{
		Log(LOG_LEVEL_WARN, "icmp fragment ");
		return iRet;
	}
	if (in_cksum((unsigned char *)picmphdr, len) != 0)
	{
		Log(LOG_LEVEL_WARN, "checksum error");
		return iRet;
	}
	return 1;
}
