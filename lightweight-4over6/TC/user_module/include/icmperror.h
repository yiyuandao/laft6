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


#ifndef _ICMPERROR_H_
#define _ICMPERROR_H_

typedef struct tag_ICMP_HDR
{
	unsigned char 		Type;
	unsigned char 		Code;
	unsigned short 		CheckSum;
	unsigned int 		Unused;
}ICMP_HDR, *PICMP_HDR;

typedef struct ICMP6_PSD_HEADER
{
	unsigned char    	sourceip6[16];    //源IP地址
	unsigned char   	destip6[16];      //目的IP地址
	unsigned char 		mbz;           //置空(0)
	unsigned char		ptcl;          //协议类型(IPPROTO_TCP)
	unsigned char		icmpl;        //ICMP包的总长度(单位:字节)
}ICMP6_PSD_HEADER, *PICMP6_PSD_HEADER;


int Icmpv6ErrorSend(unsigned char *SrcIpv6, 
							unsigned char *DstIpv6, 
							unsigned char *ErrorPayload,
							unsigned int ErrLen,
							unsigned int Count);

int IcmpErrorSend(struct icmphdr *picmphdr,
							unsigned int uiDstIPv4, 
							unsigned char *ErrorPayload,
							unsigned int ErrLen,
							unsigned int Count);
#endif
