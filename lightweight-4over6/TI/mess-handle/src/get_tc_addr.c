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
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "get_tc_addr.h"

void print_buf(unsigned char *buf, int len)
{
	int i = 0;
	for (i = 0; i < len - 1; i++)
	{
		printf("%02x ", buf[i]);
	}
	printf("%02x\n", buf[i]);
}

static struct addrinfo *resolv_hostname(char *hostname, 
						const char *service, int family)
{
	struct addrinfo hints;
	struct addrinfo *res;
	int ret = 0;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = family;
	
	ret = getaddrinfo(hostname, service, &hints, &res);
	if (ret != 0)
	{
		printf("getaddrinfo: %s\n", gai_strerror(ret));
		return NULL;
	}

	return res;
}

static void get_addr(struct addrinfo *paddrinfo, unsigned char *ipv6_addr)
{
	struct addrinfo *rp = paddrinfo;

	//for (rp = paddinfo; rp != NULL; rp = rp->ai_next)
	{
		struct sockaddr_in6 *p = (struct sockaddr_in6 *)(rp->ai_addr);
		//struct in6_addr *paddr6 = &(p->sin6_addr);
		memcpy(ipv6_addr, p->sin6_addr.s6_addr, IPV6_ADDR_LEN);
	}

	freeaddrinfo(paddrinfo);
}	

int get_tc_addr(char *hostname, unsigned char *ipv6_addr)
{
	 struct addrinfo *res = NULL;

	 assert(hostname != NULL);
	 res = resolv_hostname(hostname, "domain", AF_INET6);

	 if (res != NULL)
	 {
		 get_addr(res, ipv6_addr);
		 return 0;
	 }
	 else
	 {
		 return -1;
	 }
}

