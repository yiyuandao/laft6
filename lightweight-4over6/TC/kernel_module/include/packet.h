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
#ifndef __PACKET_H__
#define __PACKET_H__
#include <linux/icmp.h>
#define IPINIP				0X04
#define START_PORT 			1024
#define MUTI_PORT 			2
#define MINI_TIMER_VALUE	30 * 60 //30 min


//Tunnel information
typedef struct _tag_tunnel_ctx_t
{
	unsigned short usStartPort;		//begin port
	unsigned short usEndPort;		//end port
	unsigned int   uiExtIP;			//public port
	unsigned char  ucCltIPv6[16];	//client ipv6 address
	unsigned long  ulLiveTime;		//live time 
}TUNNEL_CTX, *PTUNNEL_CTX;

//align at one byte
#pragma pack(1)
typedef struct _tag_keyv6_t
{
	unsigned char ucCltIPv6[16];
}KEY_V6, *PKEY_V6;

typedef struct _tag_keyv4_t
{
	unsigned int uiExtip;
	unsigned short usPort;
}KEY_V4, *PKEY_V4;
#pragma pack()



//#define SENDSKB(n) netif_rx(n) 
#define SENDSKB(n) Dev_queue_xmit(n)

#define PACKETLEN(n) (n)->len + 14

//extern int netif_rx(struct sk_buff *skb);

int SendSkbuf(struct sk_buff *skb, unsigned short protocol);

int CreateTunnel(PTUNNEL_CTX t);

int DeleteTunnel(unsigned char *ucCltIPv6Addr, unsigned short *Port, unsigned int *ExtIP);

PTUNNEL_CTX LookupTunnel_v6(PKEY_V6 pkeyv6);

PTUNNEL_CTX LookupTunnel_v4(PKEY_V4 pkeyv4);

void HandleIPinIP_skbuf(struct sk_buff* skb);

int HandleIPv6_skbuf(struct sk_buff* skb);

int HandleIPv4_skbuf(struct sk_buff* skb);

int Handle_skbuf(struct sk_buff* skb);

int CreateIPinIP_skbuf(struct sk_buff *skb, unsigned char *ipv6address);

void TunnelTimeOut(unsigned long data);


int IPv4FragandIPinIP(struct sk_buff* skb, 
						unsigned char *ucDestIPv6);

unsigned char IsFragIPinIP(struct sk_buff* skb);

unsigned short GetFragOffset(struct sk_buff* skb,
	unsigned char *mf,
	unsigned short *length);

int IPv4FragandIPinIPExtend(struct sk_buff* skb, 
						unsigned char *ucDestIPv6);

int IPFilter(struct sk_buff* skb, unsigned short protocol);

void
patch_tcpmss(unsigned char *buf4, unsigned int len);

int IsCrashIPinPool(unsigned int extip);

int OuterICMPErr_get(struct icmphdr *picmphdr,
	unsigned short *port,
	unsigned int *extip);

int FilterIcmpErrOuter(struct icmphdr *picmphdr, unsigned int len);

void SkbufMacSet(struct sk_buff* skb, char *mac);

void Dev_queue_xmit(struct sk_buff *skb);
#endif
