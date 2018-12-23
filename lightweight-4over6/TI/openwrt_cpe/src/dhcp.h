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
#ifndef _DHCP_H_
#define _DHCP_H_
#pragma pack(1)

typedef struct _tag_DHCPv6HDR
{
	unsigned char MsgType;
	unsigned char TranId[3];
	unsigned char Msg[1];
}DHCPV6_HDR, *PDHCPV6_HDR;

typedef struct _tag_dhcpv6msghdr
{
	unsigned short option;
	unsigned short Length;
	unsigned char   Value[1];
}DHCPV6_MSG_HDR,*PDHCPV6_MSG_HDR;
#pragma pack()

typedef struct _tag_dhcpv6msg DHCPV6MSG, *PDHCPV6MSG;

typedef struct _tag_dhcpv6msg
{
	PDHCPV6MSG next;
	DHCPV6_MSG_HDR dhcpv6msghdr;
}DHCPV6_MSG_NODE, *PDHCPV6_MSG_NODE;

/*UDP伪首部-仅用于计算校验和*/
typedef struct tsd6_hdr  
{  
	unsigned char   		sourceip6[16];    //源IP地址
	unsigned char   		destip6[16];      //目的IP地址
	unsigned char  		mbz;           //置空(0)
	unsigned char  		ptcl;          //协议类型(IPPROTO_UDP)
	unsigned short 		udpl;         //UDP包总长度(单位:字节)  
}UDP6_PSD_HEADER, *PUDP6_PSD_HEADER; 

int dhcpv6_tx_captrue(struct sk_buff* skb);

int dhcpv6_rx_captrue(struct sk_buff* skb, unsigned char *dnsv6, char *aftraddr);



#endif
