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

#include <linux/netlink.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <net/net_namespace.h>
#include <net/sock.h>
#include <asm/byteorder.h>
#include "packet.h"
#include "defrag6.h"
#include "hash.h"
#include "log.h"
#include "global.h"
#include "tccorestat.h"
#define MAX_SHASH_SIZE 		256
#define FRAG_TIMEOUT 		60 //s
#define MAX_FRAG_CNT 		1144	
#define MAX_FRAG_PER_TIME 	5 //5S

PSHASHCB pshashcb = NULL;
struct timer_list fragtimer;
unsigned char frag4buf[66000] = {0};
spinlock_t fraghashlock;

static unsigned int APHash(char* str, unsigned int len)
{
	unsigned int hash = 0;
	unsigned int i    = 0;

	for(i = 0; i < len; str++, i++)
	{
		hash ^= ((i & 1) == 0) ? (  (hash <<  7) ^ (*str) ^ (hash >> 3)) :
			(~((hash << 11) ^ (*str) ^ (hash >> 5)));
	}

	return (hash & 0x7FFFFFFF);
}


static unsigned int keycompare(void *key1, void *key2, unsigned int keysize)
{
	PFRAG_TUPLE pfrag_tuple = (PFRAG_TUPLE)key1;
	PFRAG_KEY pfrag_key = (PFRAG_KEY)key2;

	if ((pfrag_tuple->id == pfrag_key->id) && 
		(memcmp(&pfrag_tuple->daddrv6, &pfrag_key->daddrv6, sizeof(pfrag_key->daddrv6)) == 0) &&
		(memcmp(&pfrag_tuple->saddrv6, &pfrag_key->saddrv6, sizeof(pfrag_key->saddrv6)) == 0))
		return 1;

	return 0;
}

static int fragcompare(void *key1, void *key2)
{
	PFRAG_CB fragcb1 = NULL, fragcb2 = NULL;
	
	fragcb1 = (PFRAG_CB)key1;
	fragcb2 = (PFRAG_CB)key2;

	if (fragcb1->offset < fragcb2->offset)
		return 1;

	if (fragcb1->offset > fragcb2->offset)
		return -1;

	//if (fragcb1->offset == fragcb2->offset)
	return 0;
}

static void fragnodedel(void *node)
{
	PFRAG_CB pfragcb = (PFRAG_CB)node;

	if (pfragcb->skb)
	{
		dev_kfree_skb(pfragcb->skb);
		pfragcb->skb = NULL;
	}

	if (pfragcb)
	{
		kfree(pfragcb);
		pfragcb = NULL;
	}
}

void createfragkey(PFRAG_KEY fragkey, struct sk_buff* skb)
{
	struct ipv6hdr* iph_v6 = NULL;
	struct frag_hdr *fragv6hdr = NULL;
	
	iph_v6 = ipv6_hdr(skb);
	fragv6hdr = (struct frag_hdr *)((unsigned char *)iph_v6 + sizeof(struct ipv6hdr));
	
	fragkey->id = fragv6hdr->identification;
	memcpy(&fragkey->daddrv6, &iph_v6->saddr, sizeof(iph_v6->saddr));
	memcpy(&fragkey->saddrv6, &iph_v6->daddr, sizeof(iph_v6->daddr));
}

PFRAG_TUPLE createfragtuple(struct sk_buff* skb, long time)
{
	struct ipv6hdr* iph_v6 = NULL;
	struct frag_hdr *fragv6hdr = NULL;
	PFRAG_TUPLE pfragtuple = NULL;
	
	iph_v6 = ipv6_hdr(skb);
	fragv6hdr = (struct frag_hdr *)((unsigned char *)iph_v6 + sizeof(struct ipv6hdr));
	pfragtuple = (PFRAG_TUPLE)kmalloc(sizeof(FRAG_TUPLE), GFP_ATOMIC);
	
	if (pfragtuple  == NULL)
		return NULL;
	
	pfragtuple->id = fragv6hdr->identification;
	memcpy(&pfragtuple->daddrv6, &iph_v6->saddr, sizeof(iph_v6->saddr));
	memcpy(&pfragtuple->saddrv6, &iph_v6->daddr, sizeof(iph_v6->daddr));
	pfragtuple->time_out = time;
	pfragtuple->flag = 0;
	pfragtuple->len = 0;
	pfragtuple->length = 0;
		
	return pfragtuple;
}

void updatetupletimeout(PFRAG_TUPLE pfragtuple, long time)
{
	pfragtuple->time_out = time;
}

int updatelength(PFRAG_TUPLE pfragtuple, struct sk_buff* skb)
{
	struct ipv6hdr* iph_v6 = NULL;
	struct frag_hdr *fragv6hdr = NULL;
	
	iph_v6 = ipv6_hdr(skb);
	fragv6hdr = (struct frag_hdr *)((unsigned char *)iph_v6 + sizeof(struct ipv6hdr));

	pfragtuple->len += (htons(iph_v6->payload_len) - sizeof(struct frag_hdr));
	
	if ((htons(fragv6hdr->frag_off) & FRAG_MORE) != 1)
	{
		pfragtuple->length = (htons(fragv6hdr->frag_off) & FRAG_OFFSET) + (htons(iph_v6->payload_len) - sizeof(struct frag_hdr));
		pfragtuple->flag = 1;
	}

	if (pfragtuple->flag)
	{
		if (pfragtuple->len == pfragtuple->length)
			return 1;
	}
	
	return -1;
}

int initfragmodule(void)
{
	pshashcb = ShashCreate(MAX_SHASH_SIZE,
		APHash,
		keycompare,
		fragcompare,
		fragnodedel);
	if (pshashcb == NULL)
		return -1;

	fraghashlockinit();
	
	fragtimerinit();

	return 1;
}

void unfragmodule(void)
{
	fragtimerstop();
	ShashDestory(pshashcb);
}

struct sk_buff* fragsk_bufcopy(struct sk_buff* skb)
{
	struct sk_buff* new_skb_buf = NULL;
	
	new_skb_buf = skb_copy(skb, GFP_ATOMIC);

	return new_skb_buf;
}

struct sk_buff * fragsk_bufalloc(unsigned int length, 
	struct net_device *dev, 
	unsigned char *mac)
{
	struct sk_buff * new_skb_buf = NULL;
	struct ethhdr* eth_t = NULL;
	new_skb_buf = dev_alloc_skb(length + ETH_HLEN +  5);
    if(!new_skb_buf)
    {
        return NULL;
    }

    skb_reserve(new_skb_buf, 2);
    skb_put(new_skb_buf, ETH_HLEN + length);

	eth_t = (struct ethhdr*)new_skb_buf->data;
	eth_t->h_proto = htons(ETH_P_IP);
	memcpy(eth_t->h_source, mac, 6);
    new_skb_buf->pkt_type = PACKET_HOST;
    new_skb_buf->protocol = htons(ETH_P_IP);
    new_skb_buf->dev = dev;

			
	skb_reset_mac_header(new_skb_buf);
    skb_pull(new_skb_buf, ETH_HLEN);
		
	return new_skb_buf;
}

struct _tag_frag_head * fragnodeinsert(PFRAG_CB pfragcb)
{
	struct _tag_bucket_node *pbucketnode = NULL;
	FRAG_KEY fragkey;
	PFRAG_TUPLE pfragtuple = NULL;
	
	if (pfragcb == NULL)
		return NULL;
	
	createfragkey(&fragkey, pfragcb->skb);
	
	pbucketnode = ShashBucketFind(pshashcb,
								&fragkey, 
								sizeof(FRAG_KEY));
	if (pbucketnode == NULL)
	{
		pfragtuple = createfragtuple(pfragcb->skb, FRAG_TIMEOUT);
		if (pfragtuple == NULL)
			return NULL;
		
		pbucketnode = ShashInsert(pshashcb,
			&fragkey,
			sizeof(FRAG_KEY),
			pfragtuple);

		pbucketnode->cnt ++;
	}
	else
	{
		pbucketnode->cnt ++;
		
		if (pbucketnode->cnt > MAX_FRAG_CNT)
			return NULL;

		updatetupletimeout((PFRAG_TUPLE)pbucketnode->pData, FRAG_TIMEOUT);
		pfragtuple = (PFRAG_TUPLE)pbucketnode->pData;
	}

	ShashFragInsert(pshashcb, &pbucketnode->fraghdr, pfragcb);

	if (updatelength((PFRAG_TUPLE)pbucketnode->pData, pfragcb->skb) > 0)
		return  &pbucketnode->fraghdr;
	
	return NULL;
}

int fragprocess(struct _tag_frag_head * fraghead)
{
	struct _tag_frag_node *pnode = NULL, *pnextnode = NULL;
	PFRAG_CB pfragcb = NULL;
	struct net_device       *dev = NULL;
	unsigned int pktlen = 0;
	unsigned char ucMac[ETH_ALEN] = {0};
	struct ethhdr* eth;

	pnode = fraghead->next;
	
	if (pnode == NULL)
		return -1;
	//
	pfragcb = (PFRAG_CB)pnode->pData;
	dev = pfragcb->skb->dev;
	eth = (struct ethhdr* )eth_hdr(pfragcb->skb);
	memcpy(ucMac, eth->h_dest, 6);
	
	while (pnode != NULL)
	{
		if (pnode->pData != NULL)
		{
			pfragcb = (PFRAG_CB)pnode->pData;

			frag_iphandle(pfragcb);
			memcpy(frag4buf + pfragcb->offset, pfragcb->skb->data, pfragcb->length);
				
			pktlen += pfragcb->length;
		
			if (pfragcb->skb)
				dev_kfree_skb(pfragcb->skb);
			
			kfree(pfragcb);
			pfragcb = NULL;
		}
		
		pnextnode = pnode->next;
		pnode->front->next = pnode->next;
		
		if (pnode->next != NULL)
			pnode->next->front = pnode->front;

		kfree(pnode);
		pnode = NULL;
		pnode = pnextnode;
	}

	
	if (pktlen > GlobalCtx.usMtu)
		pkt_fraghandle(dev, ucMac, frag4buf, pktlen);
	else
		sndipskbuf(frag4buf, pktlen, dev, ucMac);

	return 1;
}

int sndipskbuf(unsigned char *sndbuf,
	unsigned int pktlen,
	struct net_device *dev,
	unsigned char *mac)
{
	struct sk_buff *sdbuf = NULL;
	if (dev == NULL)
		return -1;
	
	sdbuf = fragsk_bufalloc(pktlen, dev, mac);
	if (sdbuf == NULL)
		return -1;

	memcpy(sdbuf->data, sndbuf, pktlen);

	skb_reset_transport_header(sdbuf);
	skb_set_transport_header(sdbuf, sizeof(struct iphdr));	
	skb_reset_network_header(sdbuf);

#if TC_STAT_DEF 				
	SetTCCoreStatInfo(TX_BYTE, PACKETLEN(sdbuf));
	SetTCCoreStatInfo(TX_PACKET, 1);
#endif

	SENDSKB(sdbuf);
	return 1;
}

void pkt_fraghandle(struct net_device *dev,
	unsigned char *mac,
	unsigned char *fragpkt,
	unsigned int pktlen)
{
	unsigned int off = 0, flen = 0, mtu = 0;
	unsigned int fmtu = 0, nfrag = 0, flen0 = 0, fleno = 0, fcc = 0;

	struct iphdr *piphdr = NULL;
	unsigned int len = 0;
	static struct iphdr ip_hdr;
	struct sk_buff *sdbuf = NULL;
	
	if (dev == NULL)
		return ;
	
	piphdr = (struct iphdr *)fragpkt;
	memcpy(&ip_hdr, piphdr, sizeof(struct iphdr));
	fragpkt += sizeof(struct iphdr);
	
	mtu = GlobalCtx.usMtu;
	len = pktlen -sizeof(struct iphdr);
	
	flen0 = 0;		/* XXX: silence compiler */
	fleno = 0;

	fmtu = (mtu - sizeof(struct iphdr)) & ~7;
	nfrag = len / fmtu;			/* always >= 1 */
			
	if (len % fmtu != 0)
		nfrag++;

	//printk("[pkt_fraghandle]:nFrag = %d\n", nfrag);
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
		if (flen > mtu - sizeof(struct iphdr)) {
			if (1) {
				if (off == 0) /* first fragment */
					flen = flen0;
				else /* intermediate fragment */
					flen = fleno;
			} else {
				flen = mtu - sizeof(struct iphdr);
				flen &= ~7;
			}
		}
		
		sdbuf = fragsk_bufalloc(flen + sizeof(struct iphdr), dev, mac);	
		if (sdbuf == NULL)
			return ;

		
		if (flen + off < len)
			ip_hdr.frag_off =  htons((off >> 3) | 0x2000); 
		else
			ip_hdr.frag_off = htons(off >> 3);
		
		ip_hdr.tot_len = htons(flen + sizeof(struct iphdr));
		ip_send_check(&ip_hdr);
		memcpy(sdbuf->data, &ip_hdr, sizeof(struct iphdr));
		memcpy(sdbuf->data + sizeof(struct iphdr), fragpkt + off, flen);
		fcc = flen +  sizeof(struct iphdr);
		
		skb_reset_transport_header(sdbuf);
		skb_set_transport_header(sdbuf, sizeof(struct iphdr));	

		skb_reset_network_header(sdbuf);
#if TC_STAT_DEF 				
		SetTCCoreStatInfo(TX_BYTE, PACKETLEN(sdbuf));
		SetTCCoreStatInfo(TX_PACKET, 1);
#endif
		SENDSKB(sdbuf);
	}
}

void frag_iphandle(PFRAG_CB pfragcb)
{
	struct sk_buff*skb = NULL;
	unsigned short offset;
	unsigned char mf;
	struct ethhdr* eth_t = NULL;
	struct ipv6hdr *pipv6hdr = NULL;

	unsigned int payload = 0;
	
	skb = pfragcb->skb;
	offset = pfragcb->offset;
	mf = pfragcb->mf;
	
	eth_t = eth_hdr(skb);
	eth_t->h_proto = htons(ETH_P_IP);
	
	pipv6hdr = ipv6_hdr(skb);
	payload = htons(pipv6hdr->payload_len);

	payload -= sizeof(struct frag_hdr);

	memmove((void *)((unsigned char *)pipv6hdr),
	(void *)((unsigned char *)pipv6hdr + sizeof(struct ipv6hdr) + sizeof(struct frag_hdr)),
	payload);
}

static void fragtimeouthandle(unsigned long data)
{
	int i = 0;
	struct _tag_bucket_node *pbucketnode = NULL, *pprebucketnode = NULL;
	PFRAG_TUPLE pfragtuple = NULL;

	Log(LOG_LEVEL_NORMAL, "fragtimeout 5s!");
	//printk("[fragtimeouthandle]:5 seconds timeout\n");
	fraghash_lock();
	for (i = 0; i < pshashcb->hashtablesize; i ++)
	{
		pprebucketnode = &pshashcb->pshashtable[i].bucketnode;
		pbucketnode = pshashcb->pshashtable[i].bucketnode.next;
		
		while (pbucketnode != NULL)
		{
			pfragtuple = (PFRAG_TUPLE)pbucketnode->pData;
			pfragtuple->time_out -= MAX_FRAG_PER_TIME;
			
			if (pfragtuple->time_out <= 0)
			{
				Log(LOG_LEVEL_NORMAL, "id :0x%x fragment packet timeout,so deleted!\n", pfragtuple->id);
				if (ShashFragDel(pshashcb, &pbucketnode->fraghdr) < 0)
				{
					Log(LOG_LEVEL_NORMAL, "fragment node is null");
				}
				pprebucketnode->next = pbucketnode->next;
				kfree(pbucketnode->pData);
				pbucketnode->pData = NULL;
				kfree(pbucketnode);
				pbucketnode = pprebucketnode->next;
			}
			else
			{
				pprebucketnode = pbucketnode;
				pbucketnode = pbucketnode->next;
			}
		}
	}	
	fraghash_unlock();

	fragtimer.expires = jiffies + MAX_FRAG_PER_TIME * HZ;
 	add_timer(&fragtimer); 
		
}

void fragtimerinit(void)
{
	init_timer(&fragtimer);

	fragtimer.data = 0;
	fragtimer.function  = fragtimeouthandle;
	fragtimer.expires = jiffies + MAX_FRAG_PER_TIME * HZ;
	add_timer(&fragtimer);
}

void fragtimerstop(void)
{
	del_timer(&fragtimer);
}

void fraghashlockinit(void)
{
	spin_lock_init(&fraghashlock);
}

void fraghash_lock(void)
{
	spin_lock(&fraghashlock);
}

void fraghash_unlock(void)
{
	spin_unlock(&fraghashlock);	
}

