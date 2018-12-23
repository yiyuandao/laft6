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


#include "precomp.h"

#define SHIFT 5     /* integer: 32 bits */
#define MASK 0x1F

/** 
 * an integer have 32 bits, one bit-->a port
 * @port: tcp or udp port pool
 * @i: tcp/udp port
 *
 */
void set(int *port, unsigned short i)
{
	// i >> SHIFT == i / 32 and i & MASK == i % 32
	if (port != NULL)
	{
		port[i >> SHIFT] |= (1 << (i & MASK));
	}
	else
	{
		printk("set: port == NULL\n");
	}
}

/**
 * the port is set ?
 * @port: tcp or udp port pool
 * @i: tcp/udp port
 *
 */
int test(int *port, unsigned short i)
{
	if (port != NULL)
	{
		return port[i >> SHIFT] & (1 << (i & MASK));
	}
	else
	{
		printk("test: port == NULL\n");
	}
}


int get_tcp_port(unsigned short low, unsigned short high)
{
	unsigned short i = 0;

	// no tcp port used ?
	if (laft_info.tcp_port_used >= high - low + 1) 
	{
		printk("failed!! get_tcp_port(): tcp_port used: %d\n", laft_info.tcp_port_used);
		return -1;
	}

	for (i = low; i <= high; i++)
	{
		// if port i not used, i - low: start from laft_info.tcpport[0]
		if (!test(laft_info.tcpport, i - low)) 
		{
			set(laft_info.tcpport, i -low);
			laft_info.tcp_port_used++;
			break;
		}
	}

	return i;
}

void reset_tcp_port(unsigned short i)
{
	int j = i - laft_info.LowPort; // convert port i to the laft_info.tcpport[];

	if (laft_info.tcpport == NULL)
	{
		printk("tcpport == NULL\n");
		return;
	}
	laft_info.tcpport[j >> SHIFT] &= ~(1 << (j & MASK));
	// add laft_info.tcp_port_used > 0 by dbgong at 2012-3-26 17:17
	if (laft_info.tcp_port_used > 0)
	{
		laft_info.tcp_port_used--;
	}
}

int get_udp_port(unsigned short low, unsigned short high)
{
	unsigned short k = 0;

	if (laft_info.udp_port_used >= high - low + 1)
	{
		printk("failed!! get_udp_port(): udp_port used: %d\n", laft_info.tcp_port_used);
		return -1;
	}

	for (k = low; k <= high; k++)
	{
		if (!test(laft_info.udpport, k -low))
		{
			set(laft_info.udpport, k - low);
			laft_info.udp_port_used++;
			break;
		}
	}

	return k;
}

void reset_udp_port(unsigned short i)
{
	int j = i - laft_info.LowPort; // convert port i to the laft_info.udpport[];

	if (laft_info.udpport == NULL)
	{
		printk("udpport == NULL\n");
		return;
	}
	laft_info.udpport[j >> SHIFT] &= ~(1 << (j & MASK));
	if (laft_info.udp_port_used > 0)
	{
		laft_info.udp_port_used--;
	}
}

unsigned short get_icmp_port(unsigned short low, unsigned short high)
{
	static unsigned short last = 0;
	static unsigned short i = 0;
	
	// first time to get icmp id
	if (i == 0) 
	{
		i = low;
		last = low;

		return i;
	}
	// new pcp request, low and high have updated
	if (last != low) 
	{
		i = low;
		return i;
	}

	i++;

	if (i > high)
	{
		i = low;
	}

	return i;
}

#ifdef __BIG_ENDIAN
int ipdst_filter(struct sk_buff *skb)
{
	struct iphdr *iph;
	unsigned int dstaddr;
	unsigned int addr2;

	iph = ip_hdr(skb);
	if(!iph)
	{
		Log(LOG_LEVEL_ERROR, "iph error");
		return NF_ACCEPT;
	}

	// in the same network
	if ((iph->saddr >> 8) == (iph->daddr >> 8))
	{
		Log(LOG_LEVEL_NORMAL, "in the same network");
		return NF_ACCEPT;
	}

	dstaddr = iph->daddr >> 24;
	addr2 = iph->daddr >> 16;

	// mcast addr: 224.0.0.0~255.255.255.255
	if ( dstaddr >= 0x000000e0)
	{
		Log(LOG_LEVEL_NORMAL, "multicast");
		return NF_ACCEPT;
	}

	// skip dest addr: 192.168.*.*;169.254.*.*;10.*.*.*;172.16-172.31.*.*
	if (addr2 == 0x0000c0a8 || addr2 == 0x0000a9fe || dstaddr == 0x0000000a || (addr2 >= 0x0000ac10 && addr2 <= 0x0000ac1f))
	{
		Log(LOG_LEVEL_NORMAL, "pravite ip");
		return NF_ACCEPT;
	}

	return -1;
}
#endif

#ifdef __LITTLE_ENDIAN
int ipdst_filter(struct sk_buff *skb)
{
	struct iphdr* iph;
	unsigned int dstaddr;
	unsigned int addr2;

	iph = ip_hdr(skb);
	if (!iph)
	{ 
		Log(LOG_LEVEL_ERROR, "little iph error");
		return NF_ACCEPT;
	} 

	// in the same network
	if ((iph->saddr << 8) == (iph->daddr << 8))
	{ 
		//Log(LOG_LEVEL_NORMAL, "little in the same network");
		return NF_ACCEPT;
	} 

	dstaddr = iph->daddr << 24;
	addr2 = iph->daddr << 16;

	// mcast addr: 224.0.0.0~255.255.255.255
	if ( dstaddr >= 0xe0000000)
	{ 
		//Log(LOG_LEVEL_NORMAL, "little multicast");
		return NF_ACCEPT;
	} 

	// skip dest addr: 192.168.*.*;169.254.*.*;10.*.*.*;172.16-172.31.*.*
	if (addr2 == 0xa8c00000 || addr2 == 0xfea90000 || dstaddr == 0x0a000000 || (addr2 >= 0x10ac0000 && addr2 <= 0x1fac0000))
	{ 
		//Log(LOG_LEVEL_NORMAL, "little pravite ip");
		return NF_ACCEPT;
	} 
	//Log(LOG_LEVEL_NORMAL, "src: %04x dst: %04x###########################\n", ntohl(iph->saddr), ntohl(iph->daddr));
	return -1;
}
#endif

void print_ether(struct ethhdr *hdr)
{
	int i;

	printk("src-mac:");
	for (i = 0; i < 5; i++)
	{   
		printk("%02x:", hdr->h_source[i]);
	}   

	printk("%02x\n", hdr->h_source[i]);

	printk("dst-mac:");
	for (i = 0; i < 5; i++)
	{   
		printk("%02x:", hdr->h_dest[i]);
	}   

	printk("%02x\n", hdr->h_dest[i]);
	printk("Proto: 0x%x\n", ntohs(hdr->h_proto)); 

}

void print_ip6addr(struct ipv6hdr *ip6head)
{
	int i;

	printk("ip6addr src:");
	for (i = 0; i < 15; i++)
	{   
		printk("%02x:", ip6head->saddr.s6_addr[i]);
	}   
	printk("%02x\n", ip6head->saddr.s6_addr[i]);

	printk("ip6addr dst:");
	for (i = 0; i < 15; i++)
	{   
		printk("%02x:", ip6head->daddr.s6_addr[i]);
	}   
	printk("%02x\n", ip6head->daddr.s6_addr[i]);
}

void print_buf(unsigned char *recvbuf, int len)
{

	int i;

	printk("buf--------------------->\n");
	for (i = 0; i < len; i++)
	{   
		printk("%02x ", recvbuf[i]);
	}   
	printk("\n");

}

