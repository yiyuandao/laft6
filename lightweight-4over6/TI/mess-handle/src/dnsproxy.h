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


#ifndef _DNS_PROXY_H
#define _DNS_PROXY_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include "global.h"
#include "log.h"

#define BUF_SIZ 4096
#define DNS_HDR_LEN 12
#define DNSA 1
#define DNSAAAA 28
#define ADDR_AND_A	  1
#define ADDR_AND_AAAA 2
#define ADDR_NO_A 	  3
#define ADDR_NO_AAAA    4
#define RECORD_EXIST 	 0
#define RECORD_NO_EXIST 3

struct Dns6Data
{
    char *buf;
    int len;
	int sockfd;
    struct sockaddr_in6 addr;
};

struct DnsHeader
{
    unsigned short id;
    unsigned short flags;
    unsigned short queries;
    unsigned short answers;
    unsigned short authors;
    unsigned short additions;
};

int dns_start6(void);

void dns_ops6(void* data);

struct Dns6Data* Dns6Data_Alloc(char *buf, int len, int sockfd, struct sockaddr_in6* addr);

void Dns6Data_Free(struct Dns6Data *data);

unsigned short checkquery(char *buf, int len, char *domainname, int* namelen);

int countlenofquery(const char* buf, int len);
unsigned char checkdns(struct DnsHeader *dnshdr);

extern NAT_CONFIG gNatConfig;


#endif
