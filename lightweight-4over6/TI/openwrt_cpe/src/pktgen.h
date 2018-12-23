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


#ifndef PKT_GEN_H_CPE
#define PKT_GEN_H_CPE

#include "precomp.h"

//extern LAFT_NAT_INFO laft_info;

int construct_eth_header(struct sk_buff* skb);
int construct_ipv6_header(struct sk_buff* skb, unsigned short len, unsigned char nexthdr);
void construct_frag_header(struct sk_buff* skb, unsigned short frag_off, unsigned int ident);
void construct_ipv6_payload(struct sk_buff *new_skb, struct sk_buff *skb, unsigned short offset);

struct sk_buff *alloc_new_skbuff(unsigned short len);

// replace the src ip and compute checksum
void update_network_header(struct sk_buff *new_skb, struct sk_buff *skb, unsigned short offset);
void update_frag_data(struct sk_buff *new_skb, struct sk_buff *skb, unsigned short offset, unsigned short len);


// udp: update the src port and checksum
void update_udp_header(struct sk_buff *new_skb, struct sk_buff *skb, unsigned short offset, unsigned short port);
// tcp: replace the src port and compute checksum
void update_tcp_header(struct sk_buff *new_skb, struct sk_buff *skb, unsigned short offset, unsigned short port);
// icmp: replace the sequence and compute checksum
void update_icmp_header(struct sk_buff *new_skb, struct sk_buff *skb, unsigned short offset, unsigned short port);
int update_transport_header(struct sk_buff *new_skb, struct sk_buff *skb, PSESSIONTUPLES session_tuple, unsigned short offset);

void set_new_skb_attr(struct sk_buff *new_skb, struct sk_buff *skb);

#endif
