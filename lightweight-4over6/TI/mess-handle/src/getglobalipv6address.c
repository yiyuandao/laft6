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
#include <string.h>
#include <error.h>
#include <ctype.h>
#include <stdlib.h>
#include <netinet/in.h>

#define IPV6_ADDR_LEN 16
#ifndef IN6_IS_ADDR_GLOBAL
#define IN6_IS_ADDR_GLOBAL(a) \
	(((((__const uint8_t *) (a))[0] & 0xff) != 0xfe   \
	  || (((__const uint8_t *) (a))[0] & 0xff) != 0xff))
#endif

static int ctodigit(char c)
{
	if (isdigit(c))
		return c - '0';
	else if (c == 'a')
		return 10;
	else if (c == 'b')
		return 11;
	else if (c == 'c')
		return 12;
	else if (c == 'd')
		return 13;
	else if (c == 'e')
		return 14;
	else //if (c == 'f')
		return 15;

}

static void string2xdigit(char *p, unsigned char *ipv6_addr)
{
	int i = 0;
	int j = 0;
	unsigned char tmp[3] = {0};

	while (i < IPV6_ADDR_LEN)
	{
		tmp[0] = ctodigit(p[j]);
		tmp[1] = ctodigit(p[j + 1]);

		ipv6_addr[i] = tmp[0] * 16 + tmp[1];

		i++;
		j += 2;
	}
}

/**
 * If head file  is not existed in your system, you could get information of IPv6 address
 * in file /proc/net/if_inet6.
 *
 * Contents of file "/proc/net/if_inet6" like as below:
 * 00000000000000000000000000000001 01 80 10 80       lo
 * fe8000000000000032469afffe08aa0f 08 40 20 80     ath0
 * fe8000000000000032469afffe08aa0f 07 40 20 80    wifi0
 * fe8000000000000032469afffe08aa0f 05 40 20 80     eth1
 * fe8000000000000032469afffe08aa0f 03 40 20 80      br0
 * fe8000000000000032469afffe08aa10 04 40 20 80     eth0
 *
 * +------------------------------+ ++ ++ ++ ++    +---+
 * |                                |  |  |  |     |
 * 1                                2  3  4  5     6
 *
 * There are 6 row items parameters:
 * 1 => IPv6 address without ':'
 * 2 => Interface index
 * 3 => Length of prefix
 * 4 => Type value (see kernel source "include/net/ipv6.h" and "net/ipv6/addrconf.c")
 * 5 => Interface flags (see kernel source "include/linux/rtnetlink.h" and "net/ipv6/addrconf.c")
 * 6 => Device name
 *
 * Note that all values of row 1~5 are hexadecimal string
 */
int get_global_ipv6addr(unsigned char ipv6_addr[][16], int m)
{
#define IF_INET6 "/proc/net/if_inet6"
	char str[128] = {0};
	char *addr;
	char *index;
	char *prefix; 
	char *scope;
	char *flags;
	char *name;
	char *delim = " \t\n";
	char *p;
	FILE *fp;
	int count;
	int i = 0;

	if (NULL == (fp = fopen(IF_INET6, "r"))) {
		perror("fopen error");
		return -1;
	}

#define IPV6_ADDR_GLOBAL 0x0000U
	while (fgets(str, sizeof(str), fp)) {
		//DG("str:%s", str);
		addr = strtok(str, delim);
		index = strtok(NULL, delim);
		prefix = strtok(NULL, delim);
		scope = strtok(NULL, delim);
		flags = strtok(NULL, delim);
		name = strtok(NULL, delim);
#if 0
		DG("addr:%s, index:0x%s, prefix:0x%s, scope:0x%s, flags:0x%s, name:%s\n",
				addr, index, prefix, scope, flags, name);

		if (strcmp(name, iface))
			continue;
#endif

		/* Just get IPv6 global address */
		if (IPV6_ADDR_GLOBAL != (unsigned int)strtoul(scope, NULL, 16))
			continue;
#if 0
		/* skip eui64 address*/
		if ( (addr[22] == 'f') && (addr[23] == 'f') && 
				(addr[24] == 'f') && (addr[25] == 'e'))
		{
			continue;
		}
#endif
		p = addr;
		string2xdigit(p, ipv6_addr[i++]);
		//break;
	}
#undef IPV6_ADDR_GLOBAL

	fclose(fp);
	return i;
}

#if 0 
void print_buf(unsigned char *buf, int len)
{
	int i = 0;

	while (i < len -1)
	{
		printf("%02x ", buf[i]);
		i++;
	}
	printf("%02x\n", buf[i]);
}
int main (int argc, char *argv[])
{
	struct in6_addr addr6;
	char *ifname = argv[1];
	unsigned char address[4][16] = {0};
	int val;

	if (get_global_ipv6addr(&addr6, ifname, address)) {
		printf("\nGet IPv6 global address of %s failed :(\n", ifname);
		return -1;
	}

	print_buf(address[0], 16);
	print_buf(address[1], 16);
	return 0;
}
#endif
