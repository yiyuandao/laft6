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
#include <linux/in6.h>
#include <linux/in.h>
#include <linux/ipv6.h>
#include <net/ipv6.h>
#include <net/route.h>
#include <net/flow.h>
#include <net/arp.h>
#include <net/neighbour.h>
#include <net/ip6_route.h>
#include <linux/ip.h>
#include <linux/string.h>
#include "mac.h"
#include "log.h"

unsigned char *lookup_ipv6_dest_mac(struct sk_buff *new_skb)
{
	struct ipv6hdr *ipv6h;
	//struct flowi fl;

	ipv6h = ipv6_hdr(new_skb);
	if(!ipv6h)
	{
		Log(LOG_LEVEL_ERROR, "ipv6h error");
		return NULL;
	}

	struct flowi fl = {
		.nl_u = {
			.ip6_u = {
				.daddr = ipv6h->daddr 
			},
		},
	};

	// lookup ipv6 route
	struct dst_entry *pdst = ip6_route_output(&init_net, NULL, &fl);
	if (pdst != NULL)
	{
		if (!pdst->error)
		{
			new_skb->dev = pdst->dev;
			//set_source_mac(new_skb, new_skb->dev->dev_addr);
			return pdst->neighbour->ha;
		}
		else
		{
			Log(LOG_LEVEL_ERROR, "error not found route!!!!!!!!!!!!!!!!!!");
			return NULL;
		}
	}
	else
	{
		Log(LOG_LEVEL_ERROR, "not found route!!!!!!!!!!!!!!!!!!");
		return NULL;
	}
}

unsigned char *lookup_ipv4_dest_mac(struct sk_buff *new_skb)
{
	struct iphdr *iph;
	struct rtable *rt;

	iph = ip_hdr(new_skb);
	if(!iph)
	{
		Log(LOG_LEVEL_ERROR, "iph error");
		return NULL; 
	}

	struct flowi fl = { 
		.nl_u = { 
			.ip4_u = { 
				.daddr = iph->daddr
			},
		}, 
	};

	// lookup ipv4 route
	if (ip_route_output_key(&init_net, &rt, &fl) != 0)
	{
		Log(LOG_LEVEL_ERROR, "not found  ipv4 route---------------------######");
		return NULL;
	}

	new_skb->dev = rt->u.dst.dev;
	//set_source_mac(new_skb, new_skb->dev->dev_addr);
	return rt->u.dst.neighbour->ha;
#if 0
	__be32 dest = 0;
	dest = rt->rt_gateway;
	struct neighbour *neighbor_entry = neigh_lookup(&arp_tbl, &dest, new_skb->dev);

	if (neighbor_entry != NULL)
	{
		return neighbor_entry->ha;
	}
	else
	{
		Log(LOG_LEVEL_ERROR, "not found  mac addr---------------------######");
		return NULL;
	}
#endif
}

void set_dest_mac(struct sk_buff *skb, unsigned char *dest_mac)
{
	struct ethhdr *eth = eth_hdr(skb);
	memcpy(eth->h_dest, dest_mac, ETH_ALEN);
}
void set_source_mac(struct sk_buff *skb, unsigned char *source_mac)
{
	struct ethhdr *eth = eth_hdr(skb);
	memcpy(eth->h_source, source_mac, ETH_ALEN);
}
#if 0
void Dev_queue_xmit(struct sk_buff *skb)
{
	unsigned char *dest_mac = lookup_ipv6_dest_mac(skb);
	if (dest_mac != NULL)
	{   
		set_dest_mac(skb, dest_mac);
	}   
	else
	{   
		kfree_skb(skb);
		return;
	}   

	skb_push(skb, ETH_HLEN);
	dev_queue_xmit(skb);
}
#endif
