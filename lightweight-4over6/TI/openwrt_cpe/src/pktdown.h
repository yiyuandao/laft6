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


#ifndef _PKTDOWN_H_
#define _PKTDOWN_H_

#define FRAG_OFFSET 	0xfffe
#define FRAG_MORE	0x0001

typedef struct TCP_PSD_HEADER{
	unsigned int   		sourceip;    //源IP地址
	unsigned int   		destip;      //目的IP地址
	unsigned char		mbz;           //置空(0)
	unsigned char 		ptcl;          //协议类型(IPPROTO_TCP)
	unsigned short 		tcpl;        //TCP包的总长度(单位:字节)
}TCP_PSD_HEADER, *PTCP_PSD_HEADER;

/*UDP伪首部-仅用于计算校验和*/
typedef struct tsd_hdr  
{  
	unsigned int  		sourceip;    //源IP地址
	unsigned int  		destip;      //目的IP地址
	unsigned char  		mbz;           //置空(0)
	unsigned char  		ptcl;          //协议类型(IPPROTO_UDP)
	unsigned short 		udpl;         //UDP包总长度(单位:字节)  
}UDP_PSD_HEADER, *PUDP_PSD_HEADER; 

int skbuf_handle(struct sk_buff* skb);

int ipv6handle(struct sk_buff* skb);

void  ipiphandle(struct sk_buff*skb);

int nathandle(struct sk_buff* skb);

void pktchecksum(struct sk_buff* skb, PSESSIONTUPLES psessiontuples);

unsigned char isfragipip(struct sk_buff* skb);

unsigned short getfragoffset(struct sk_buff* skb, unsigned char *mf, unsigned short *length);

int OuterICMPErr_get(struct icmphdr *picmphdr,
	unsigned short *port,
	unsigned int *extip,
	unsigned char *protocol);

void OuterICMPErr_set(struct icmphdr *picmphdr, 
	unsigned int inip, 
	unsigned short port);

int IsOutICMPErrPacket(struct iphdr *piphdr);

void OutICMPErrCheckSum(struct iphdr *piphdr);

#endif


