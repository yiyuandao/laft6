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
#include <unistd.h>
#include <sys/socket.h>
#include <linux/icmp.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>



static unsigned short ip_checksum(unsigned short * buffer, int size) 
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

int IcmpErrorSend(unsigned int uiSrcIPv4, 
							unsigned int uiDstIPv4, 
							unsigned char *ErrorPayload,
							unsigned int ErrLen,
							unsigned int Count)
{
	unsigned char sendbuf[2000] = {0};
	int s = 0;
	int i = 0;
	struct sockaddr_in saServer; 
	struct icmphdr *picmphdr = NULL;
	int iRet = 0;
	int sendlen = 0;
	struct timeval nTvlen={0, 200}; 
	
	s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (s == -1)
	{
		return -1;
	}
	iRet = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, 
		(char*)&nTvlen, sizeof(nTvlen));
	if (iRet == -1) 
	{
		close(s);
		return -1;
	}
	
	memset(&saServer, 0, sizeof(saServer));
	
	saServer.sin_family = AF_INET;
	saServer.sin_addr.s_addr = htonl(uiDstIPv4);
	while (i < Count)
	{
		sendlen = sizeof(struct icmphdr) + ErrLen;

		picmphdr = (struct icmphdr *)sendbuf;
		picmphdr->checksum = 0;
		picmphdr->type = 3;
		picmphdr->code = 4;
		picmphdr->un.frag.mtu = htons(1280);
		if (ErrLen > (sizeof(sendbuf) - sizeof(struct icmphdr)))
		{
			close(s);
			return -1;
		}	
		memcpy(sendbuf + sizeof(struct icmphdr), ErrorPayload, ErrLen);
		
		picmphdr->checksum = ip_checksum((unsigned short *)sendbuf, sendlen);

		iRet = sendto(s,
			sendbuf,
			sendlen,
			0, 
			(struct sockaddr *)&saServer,
			sizeof(saServer));
		if (iRet < 0)
		{
			close(s);
			return -1;
		}
		i ++;
	}

	close(s);
	return iRet;
}

