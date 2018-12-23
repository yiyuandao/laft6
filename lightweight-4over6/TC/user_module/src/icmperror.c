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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <linux/icmpv6.h>
#include <linux/icmp.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "log.h"
#include "icmperror.h"


unsigned short ip_checksum(unsigned short * buffer, int size) 
{
	unsigned long cksum = 0;

	// Sum all the words together, adding the final byte if size is odd

	while (size > 1) 
	{
		cksum += *buffer++;
		size -= sizeof(unsigned short);
	}
	if (size)
	{
		cksum += *(unsigned char *)buffer;
	}

	// Do a little shuffling

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);

	// Return the bitwise complement of the resulting mishmash

	return (unsigned short)(~cksum);
}

int Icmpv6ErrorSend(unsigned char *SrcIpv6, 
							unsigned char *DstIpv6, 
							unsigned char *ErrorPayload,
							unsigned int ErrLen,
							unsigned int Count)
{
	unsigned char sendbuf[2000] = {0};
	unsigned char chksmbuf[2000] = {0};
	int s = 0;
	int i = 0;
	struct sockaddr_in6 Server6Addr_t;
	PICMP_HDR picmphdr = NULL;
	PICMP6_PSD_HEADER picmppsdhdr = NULL;
	int iRet = 0;
	int sendlen = 0;
	struct timeval nTvlen={0, 200}; 
	
	s = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if (s == -1)
	{
		printf("%s:socket error\n", __FUNCTION__);
		return -1;
	}
	iRet = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, 
		(char*)&nTvlen, sizeof(nTvlen));
	if (iRet == -1) 
	{
		close(s);
		printf("%s:setsockopt error\n", __FUNCTION__);
		return -1;
	}
	memset(&Server6Addr_t, 0, sizeof(Server6Addr_t));

	Server6Addr_t.sin6_family = AF_INET6;
	memcpy(Server6Addr_t.sin6_addr.s6_addr, DstIpv6, 16);
	
	while (i <= Count)
	{
		sendlen = sizeof(ICMP_HDR) + ErrLen;
		picmppsdhdr = (PICMP6_PSD_HEADER)chksmbuf;
		picmppsdhdr->icmpl = htons(sendlen);
		picmppsdhdr->mbz = 0;
		picmppsdhdr->ptcl = 0x3a;
		memcpy(picmppsdhdr->destip6, DstIpv6, 16);
		memcpy(picmppsdhdr->sourceip6, SrcIpv6, 16);
		picmphdr = (PICMP_HDR)sendbuf;
		picmphdr->CheckSum = 0;
		picmphdr->Type = 1;
		picmphdr->Code = 0;
		picmphdr->Unused = 0xffffffff;
		if (ErrLen > (sizeof(sendbuf) - sizeof(ICMP_HDR)))
		{
			printf("%s:length error\n", __FUNCTION__);

			close(s);
			return -1;
		}	
		memcpy(sendbuf + sizeof(ICMP_HDR), ErrorPayload, ErrLen);
		memcpy(chksmbuf + sizeof(ICMP6_PSD_HEADER), sendbuf, sendlen);
		
		picmphdr->CheckSum = (ip_checksum((unsigned short *)chksmbuf, sendlen + sizeof(ICMP6_PSD_HEADER)));
		iRet = sendto(s,
			sendbuf,
			sendlen,
			0, 
			(struct sockaddr *)&Server6Addr_t,
			sizeof(Server6Addr_t));
		if (iRet < 0)
		{
			printf("%s:sendto error\n", __FUNCTION__);
			close(s);
			return -1;
		}
		//sleep(1);
		i ++;
	}

	close(s);
	return iRet;
}



int IcmpErrorSend(struct icmphdr *picmphdr,
							unsigned int uiDstIPv4, 
							unsigned char *ErrorPayload,
							unsigned int ErrLen,
							unsigned int Count)
{
	unsigned char sendbuf[2000] = {0};
	unsigned char chksmbuf[2000] = {0};
	int s = 0;
	int i = 0;
	struct sockaddr_in saServer; 
	int iRet = 0;
	int sendlen = 0;
	struct timeval nTvlen={0, 200}; 
	struct icmphdr *phdr = NULL;
	
	s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (s == -1)
	{
		Log(LOG_LEVEL_ERROR, "socket error");
		return -1;
	}
	iRet = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, 
		(char*)&nTvlen, sizeof(nTvlen));
	if (iRet == -1) 
	{
		close(s);
		Log(LOG_LEVEL_ERROR, "setsockopt error");
		return -1;
	}
	
	memset(&saServer, 0, sizeof(saServer));
	
	saServer.sin_family = AF_INET;
	saServer.sin_addr.s_addr = htonl(uiDstIPv4);
	
	while (i < Count)
	{
		sendlen = sizeof(struct icmphdr) + ErrLen;
		if (ErrLen > (sizeof(sendbuf) - sizeof(struct icmphdr)))
		{
			Log(LOG_LEVEL_ERROR, "length error");
			close(s);
			return -1;
		}	
		memcpy(sendbuf, picmphdr, sizeof(struct icmphdr));
		memcpy(sendbuf + sizeof(struct icmphdr), ErrorPayload, ErrLen);
		phdr = (struct icmphdr *)sendbuf;
		phdr->checksum = 0;
		phdr->checksum = ip_checksum((unsigned short *)sendbuf, sendlen);
		iRet = sendto(s,
			sendbuf,
			sendlen,
			0, 
			(struct sockaddr *)&saServer,
			sizeof(saServer));
		if (iRet < 0)
		{
			Log(LOG_LEVEL_ERROR, "sendto error");
			close(s);
			return -1;
		}

		i ++;
	}

	close(s);
	return iRet;
}

