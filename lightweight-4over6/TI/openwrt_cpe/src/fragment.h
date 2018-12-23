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


#ifndef _FRAGMENT_H_
#define _FRAGMENT_H_



//daddr,saddr,protocol,id
typedef struct _tag_frag_tuple
{
	struct in6_addr daddrv6;
	struct in6_addr saddrv6;
	unsigned int id;
	unsigned int len;
	unsigned int length;
	unsigned char flag;
	long time_out;		
}FRAG_TUPLE, *PFRAG_TUPLE;

#pragma pack(1)
typedef struct _tag_frag_key
{
	struct in6_addr daddrv6;
	struct in6_addr saddrv6;
	unsigned int id;
}FRAG_KEY, *PFRAG_KEY;
#pragma pack()

typedef struct _tag_frag_cb
{
	unsigned short offset;
	unsigned char mf;
	unsigned short length;
	struct sk_buff* skb;
}FRAG_CB, *PFRAG_CB;

typedef struct _tag_frag_nat_cb
{
	unsigned short offset;
	unsigned int length;
}FRAG_NAT_CB, *PFRAG_NAT_CB;

struct sk_buff* fragsk_bufcopy(struct sk_buff* skb);
void createfragkey(PFRAG_KEY fragkey, struct sk_buff* skb);
PFRAG_TUPLE createfragtuple(struct sk_buff* skb, long time);
void updatetupletimeout(PFRAG_TUPLE pfragtuple, long time);
int updatelength(PFRAG_TUPLE pfragtuples, struct sk_buff* skb);
int initfragmodule(void);
void unfragmodule(void);
struct _tag_frag_head * fragnodeinsert(PFRAG_CB pfragcb);
int fragprocess(struct _tag_frag_head * fraghead);
void  frag_ipiphandle(PFRAG_CB pfragcb);
int frag_nathandle(struct sk_buff*skb, unsigned short offset);
void pkt_fraghandle(struct net_device *dev, unsigned char *fragpkt, unsigned int pktlen);

void frag_iphandle(PFRAG_CB pfragcb);

void fragtimerinit(void);
void fragtimerstop(void);
void fraghashlockinit(void);
void fraghash_lock(void);
void fraghash_unlock(void);

#endif
