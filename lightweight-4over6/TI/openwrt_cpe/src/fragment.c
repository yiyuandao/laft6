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

//max hash table size
#define MAX_SHASH_SIZE 256
//fragment node time out
#define FRAG_TIMEOUT 30
//max fragment count
#define MAX_FRAG_CNT 1024	

//single key value hash table control block
PSHASHCB pshashcb = NULL;
//fragment timer
struct timer_list fragtimer;
//fragment reassembly buffer 
unsigned char frag4buf[66000] = {0};
//fragment hash locker
spinlock_t fraghashlock;

//compare the hash key value 
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

//compare the fragment offset value 
static int fragcompare(void *key1, void *key2)
{
	PFRAG_CB fragcb1 = NULL, fragcb2 = NULL;
	
	fragcb1 = (PFRAG_CB)key1;
	fragcb2 = (PFRAG_CB)key2;

	if (fragcb1->offset < fragcb2->offset)
		return 1;

	if (fragcb1->offset > fragcb2->offset)
		return -1;

	return 0;
}

//delete the fragment node
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

//create the fragment key value
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

//create the fragment tuple 
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

//update the tuple timeout value
void updatetupletimeout(PFRAG_TUPLE pfragtuple, long time)
{
	pfragtuple->time_out = time;
}

//The length of the update received
int updatelength(PFRAG_TUPLE pfragtuple, struct sk_buff* skb)
{
	struct ipv6hdr* iph_v6 = NULL;
	struct frag_hdr *fragv6hdr = NULL;
	
	iph_v6 = ipv6_hdr(skb);
	fragv6hdr = (struct frag_hdr *)((unsigned char *)iph_v6 + sizeof(struct ipv6hdr));

	pfragtuple->len += (htons(iph_v6->payload_len) - sizeof(struct frag_hdr));

	//judge whether a divided for the final fragment packet
	if ((htons(fragv6hdr->frag_off) & FRAG_MORE) != 1)
	{
		pfragtuple->length = (htons(fragv6hdr->frag_off) & FRAG_OFFSET) + (htons(iph_v6->payload_len) - sizeof(struct frag_hdr));
		pfragtuple->flag = 1;
	}

	if (pfragtuple->flag)
	{
		//receive complete	
		if (pfragtuple->len == pfragtuple->length)
			return 1;
	}
	
	return -1;
}

//initialize the fragment module
int initfragmodule(void)
{
	//initialize the hash table
	pshashcb = ShashCreate(MAX_SHASH_SIZE,
		APHash,
		keycompare,
		fragcompare,
		fragnodedel);
	if (pshashcb == NULL)
		return -1;
	//initialize the fragment hash table locker
	fraghashlockinit();
	//initialize the fragment timer
	fragtimerinit();

	return 1;
}

//uninstall fragment module
void unfragmodule(void)
{
	//stop the fragment timer
	fragtimerstop();
	//destory hash table
	ShashDestory(pshashcb);
}

//clone the fragment packet
struct sk_buff* fragsk_bufcopy(struct sk_buff* skb)
{
	struct sk_buff* new_skb_buf = NULL;
	
	new_skb_buf = skb_copy(skb, GFP_ATOMIC);

	return new_skb_buf;
}

//alloc fragment sk_buf
struct sk_buff * fragsk_bufalloc(unsigned int length, struct net_device *dev)
{
	struct sk_buff * new_skb_buf = NULL;
	
	new_skb_buf = dev_alloc_skb(length + ETH_HLEN +  5);
    if(!new_skb_buf)
    {
       return NULL;
    }

    skb_reserve(new_skb_buf, 2);
    skb_put(new_skb_buf, ETH_HLEN + length);

    new_skb_buf->pkt_type = PACKET_HOST;
    new_skb_buf->protocol = htons(ETH_P_IP);
    new_skb_buf->dev = dev;
		
	skb_reset_mac_header(new_skb_buf);
    skb_pull(new_skb_buf, ETH_HLEN);
		
	return new_skb_buf;
}

//insert a fragment node
struct _tag_frag_head * fragnodeinsert(PFRAG_CB pfragcb)
{
	struct _tag_bucket_node *pbucketnode = NULL;
	FRAG_KEY fragkey;
	PFRAG_TUPLE pfragtuple = NULL;
	
	if (pfragcb == NULL)
		return NULL;
	
	//create fragment key
	createfragkey(&fragkey, pfragcb->skb);
	//find fragment head node 
	pbucketnode = ShashBucketFind(pshashcb,
								&fragkey, 
								sizeof(FRAG_KEY));
	if (pbucketnode == NULL)
	{
		//create a new fragment tuple
		pfragtuple = createfragtuple(pfragcb->skb, FRAG_TIMEOUT);
		if (pfragtuple == NULL)
			return NULL;
		//insert node in hash table
		pbucketnode = ShashInsert(pshashcb,
			&fragkey,
			sizeof(FRAG_KEY),
			pfragtuple);
		//increase the node count 
		pbucketnode->cnt ++;
	}
	else
	{
		pbucketnode->cnt ++;
		//check the node count
		if (pbucketnode->cnt > MAX_FRAG_CNT)
			return NULL;
		//update the tuple node timeout value
		updatetupletimeout((PFRAG_TUPLE)pbucketnode->pData, FRAG_TIMEOUT);
	}
	//insert new fragment node in hash table
	ShashFragInsert(pshashcb, &pbucketnode->fraghdr, pfragcb);

	//update the length 
	if (updatelength((PFRAG_TUPLE)pbucketnode->pData, pfragcb->skb) > 0)
		return  &pbucketnode->fraghdr;
	
	return NULL;
}

//reassemble the fragment packet 
int fragprocess(struct _tag_frag_head * fraghead)
{
	struct _tag_frag_node *pnode = NULL, *pnextnode = NULL;
	PFRAG_CB pfragcb = NULL;
	struct net_device       *dev = NULL;
	unsigned int pktlen = 0;
	
	pnode = fraghead->next;
	
	while (pnode != NULL)
	{
		if (pnode->pData != NULL)
		{
			pfragcb = (PFRAG_CB)pnode->pData;
			//the ipv6 packet convert the ipv4 packet
			frag_iphandle(pfragcb);
			memcpy(frag4buf + pfragcb->offset, pfragcb->skb->data, pfragcb->length);
			dev = pfragcb->skb->dev;
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
	
	//fragment the new packet
	pkt_fraghandle(dev, frag4buf, pktlen);
	
	return 1;
}

//fragment the new packet and send 
void pkt_fraghandle(struct net_device *dev, unsigned char *fragpkt, unsigned int pktlen)
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
	
	mtu = 1500;
	len = pktlen - sizeof(struct iphdr);
	
	flen0 = 0;		/* XXX: silence compiler */
	fleno = 0;

	fmtu = (mtu - sizeof(struct iphdr)) & ~7;
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

		//allocate the new send sk_buffer
		sdbuf = fragsk_bufalloc(flen + sizeof(struct iphdr), dev);	
		if (sdbuf == NULL)
			return ;
		if (flen + off < len)
			ip_hdr.frag_off =  htons((off >> 3) | 0x2000); 
		else
			ip_hdr.frag_off = htons(off >> 3);
		
		ip_hdr.tot_len = htons(flen + sizeof(struct iphdr));
		//fill the new sk_buffer
		memcpy(sdbuf->data, &ip_hdr, sizeof(struct iphdr));
		memcpy(sdbuf->data + sizeof(struct iphdr), fragpkt + off, flen);
		fcc = flen +  sizeof(struct iphdr);
		
		sdbuf->len = fcc;
		skb_reset_transport_header(sdbuf);
		skb_set_transport_header(sdbuf, sizeof(struct iphdr));
		skb_reset_network_header(sdbuf);
		//ipv4 nat
		frag_nathandle(sdbuf, off);
		//send the new sk_buffer
		//netif_rx(sdbuf);
		SENDSKB(sdbuf);
	}
}

//the ipv6 packet convert the ipv4 packet
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
	//move the ipv4 head to the ipv6 head
	memmove((void *)((unsigned char *)pipv6hdr),
	(void *)((unsigned char *)pipv6hdr + sizeof(struct ipv6hdr) + sizeof(struct frag_hdr)),
	payload);
}

//create the ipinip packet
void  frag_ipiphandle(PFRAG_CB pfragcb)
{
	struct sk_buff*skb = NULL;
	unsigned short offset;
	unsigned char mf;
	struct ethhdr* eth_t = NULL;
	struct ipv6hdr *pipv6hdr = NULL;
	static struct iphdr iphdr; 
	struct iphdr *piphdr = NULL;
	unsigned int payload = 0;

	skb = pfragcb->skb;
	offset = pfragcb->offset;
	mf = pfragcb->mf;
	
	eth_t = eth_hdr(skb);
	eth_t->h_proto = htons(ETH_P_IP);
	
	pipv6hdr = ipv6_hdr(skb);
	payload = htons(pipv6hdr->payload_len);

	payload -= sizeof(struct frag_hdr);
	
	if (offset == 0)
	{
		memmove((void *)pipv6hdr,
		(void *)((unsigned char *)pipv6hdr + sizeof(struct ipv6hdr) + sizeof(struct frag_hdr)),
		payload);
		
		piphdr = (struct iphdr *)skb->data;
		memcpy(&iphdr, piphdr, sizeof(struct iphdr));

		
		piphdr->frag_off = htons(offset | 0x2000);
		
		piphdr->tot_len = htons(payload);
		
		skb->protocol = htons(ETH_P_IP);
		skb->pkt_type = PACKET_HOST;
		skb->tail -= (sizeof(struct ipv6hdr) + sizeof(struct frag_hdr));
		skb->len -= (sizeof(struct ipv6hdr) + sizeof(struct frag_hdr));	
	}
	else
	{
		memmove((void *)((unsigned char *)pipv6hdr +  sizeof(struct iphdr)),
		(void *)((unsigned char *)pipv6hdr + sizeof(struct ipv6hdr) + sizeof(struct frag_hdr)),
		payload);
		
		memcpy((unsigned char *)pipv6hdr, &iphdr, sizeof(struct iphdr));
		piphdr = (struct iphdr *)((unsigned char *)pipv6hdr);

		offset -= sizeof(struct iphdr);

		if (mf)
			piphdr->frag_off = htons(offset | 0x2000);
		else
			piphdr->frag_off  = htons(offset >> 3);	
		piphdr->tot_len = htons(payload + sizeof(struct iphdr));
		
		skb->protocol = htons(ETH_P_IP);
		skb->pkt_type = PACKET_HOST;
		skb->tail -= (sizeof(struct ipv6hdr) + sizeof(struct frag_hdr));
		skb->tail += sizeof(struct iphdr);
		skb->len -= (sizeof(struct ipv6hdr) + sizeof(struct frag_hdr));
		skb->len += sizeof(struct iphdr);
	}
	
	skb_reset_transport_header(skb);
	skb_set_transport_header(skb, sizeof(struct iphdr));
} 

//the fragment packet nat 
int frag_nathandle(struct sk_buff*skb, unsigned short offset)
{
	static unsigned int destip = 0;
	struct iphdr *piphdr = NULL;
	
	if (offset == 0)
	{
		//NAT
		if (nathandle(skb) < 0)
			return -1;
		piphdr =  (struct iphdr *)(skb->data);
		destip = piphdr->daddr;
	}
	else
	{
		piphdr =  (struct iphdr *)(skb->data);
		piphdr->daddr = destip;
	}
	//checksum
	ip_send_check(piphdr);
	return 1;
}

//check the fragment node timeout 
static void fragtimeouthandle(unsigned long data)
{
	int i = 0;
	struct _tag_bucket_node *pbucketnode = NULL, *pprebucketnode = NULL;
	PFRAG_TUPLE pfragtuple = NULL;

	//Log(LOG_LEVEL_NORMAL, "fragtimeout 5s!");
	fraghash_lock();
	for (i = 0; i < pshashcb->hashtablesize; i ++)
	{
		pprebucketnode = &pshashcb->pshashtable[i].bucketnode;
		pbucketnode = pshashcb->pshashtable[i].bucketnode.next;
		
		while (pbucketnode != NULL)
		{
			pfragtuple = (PFRAG_TUPLE)pbucketnode->pData;
			pfragtuple->time_out -= 5;
			
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

	fragtimer.expires = jiffies + 5 * HZ;
 	add_timer(&fragtimer); 		
}

//initialize fragment timer
void fragtimerinit(void)
{
	init_timer(&fragtimer);

	fragtimer.data = 0;
	fragtimer.function  = fragtimeouthandle;
	fragtimer.expires = jiffies + 5 * HZ;
	add_timer(&fragtimer);
}

//stop fragment timer
void fragtimerstop(void)
{
	del_timer(&fragtimer);
}

//initialize fragment hash locker 
void fraghashlockinit(void)
{
	spin_lock_init(&fraghashlock);
}

//lock
void fraghash_lock(void)
{
	spin_lock(&fraghashlock);
}

//unlock
void fraghash_unlock(void)
{
	spin_unlock(&fraghashlock);	
}
