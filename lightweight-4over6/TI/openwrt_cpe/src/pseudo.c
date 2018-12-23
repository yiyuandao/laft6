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
#include <linux/string.h>
#include <linux/skbuff.h>
#include <linux/tcp.h>
#include <linux/ip.h>
#include "pseudo.h"
#include "packet.h"
#include "helper.h"

#if 0
//checksum algorithm
unsigned short ip_checksum(unsigned short* buffer, int size)
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
		cksum += *(unsigned char*)buffer;
	}

	// Do a little shuffling

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);

	// Return the bitwise complement of the resulting mishmash

	return (unsigned short)(~cksum);
}
#endif
int calc_tcp_checksum(struct sk_buff *newskb)
{
	struct tcphdr *ptcp_header = NULL; //UDP头指针
	struct iphdr *piph = NULL; //UDP头指针
	PPSD_HEADER ptcp_psd_header = NULL;
	unsigned short attachsize = 0; 
	int i = 0;

	while (i < newskb->len - 1)
	{
		printk("%02x ", newskb->data[i++]);
	}
	printk("%02x ", newskb->data[i++]);

	piph = (struct ipv6hdr *)newskb->data;
	ptcp_header = (struct tcphdr *)(newskb->data + sizeof(struct iphdr));
	attachsize = htons(piph->tot_len) - (piph->ihl << 2);
	ptcp_psd_header = kmalloc(1500, GFP_ATOMIC);
	if (ptcp_psd_header == NULL)
		return -1;
	memset(ptcp_psd_header, 0, attachsize + sizeof(PSD_HEADER));
	memcpy(&ptcp_psd_header->destip, &piph->daddr, 4);
	memcpy(&ptcp_psd_header->sourceip, &piph->saddr, 4);
	ptcp_psd_header->mbz = 0;
	ptcp_psd_header->ptcl = IPPROTO_TCP;
	ptcp_psd_header->payloadlen = htons(attachsize);
	ptcp_header->check = 0;
	memcpy((unsigned char *)ptcp_psd_header + sizeof(PSD_HEADER),
			(unsigned char *)ptcp_header, attachsize);
	ptcp_header->check = ip_checksum((unsigned short *)ptcp_psd_header,
			attachsize + sizeof(PSD_HEADER));
	kfree(ptcp_psd_header);

	return 0;
}
