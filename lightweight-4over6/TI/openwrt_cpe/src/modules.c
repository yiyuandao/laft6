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

//initialize modules
unsigned int modules_init(void)
{
	// init netlink socket
	Netlink_kernel_create();

	//initialize session hash locker
	spin_lock_init(&hashlock);
	//create the session hash table
	pnathash = DBKeyHashTableCreate(MAX_ENTRY,
		APHash,
		compare_session,
		sessiondatproc,
		DataDel,
		NULL);
	if (pnathash == NULL)
	{
		Log(LOG_LEVEL_ERROR, "can't create hash table");
		return 0;
	}
	//initialize the session timer
	Initsessiontimer();
	//initialize fragment module
	initfragmodule();

	return 1;
	
}

//uninitialize modules
unsigned int modules_uninit(void)
{
	// release netlink socket
	Netlink_kernel_release();

	//un//initialize fragment modules
	unfragmodule();
	//stop session timer
	stopsessiontimer();
	//destory hash table
	DBKeyHashTableDestory(pnathash);

	return 1;
}
static int stream_len_check(struct sk_buff* skb)
{
	struct iphdr* iph = NULL;
	unsigned short frag = 0;
	
	iph = ip_hdr(skb);
	if(!iph)
	{   
		Log(LOG_LEVEL_ERROR, "iph error");
		return -1; 
	}  
	
	if ((iph->frag_off & htons(IP_DF)) && 
		skb->len + IP6_HLEN > 1280)
	{	
		Log(LOG_LEVEL_WARN, "DF is set and len more than 1280");
		send_nlmsg(async_nl_sk, get_icmp_error_pid(), skb->data, 28);

		return -1;	
	}
	
	return 1;
}
//handle downside packets
unsigned int stream_down_hook(unsigned int hooknum, 
	struct sk_buff* skb,
	const struct net_device* in,
	const struct net_device* out, 
	int(*okfn)(struct sk_buff*))
{
	int ret = 0;
	struct sk_buff* new_skb_buf = NULL;
	struct ethhdr* eth;

	//printk("stream_down_hook started\n");	
	//filter the skbuffer 
	ret = skbuf_handle(skb);
	if (ret < 0)
		//drop the packet
		return NF_DROP;

	if (ret == 0)
	{
		//allocate the sk buffer
		new_skb_buf = dev_alloc_skb(skb->len + ETH_HLEN +  5);
        if(!new_skb_buf)
        {
            return NF_DROP;
        }

		//fill the sk buffer field
        skb_reserve(new_skb_buf, 2);
        skb_put(new_skb_buf, ETH_HLEN + skb->len);

        new_skb_buf->pkt_type = PACKET_HOST;
        new_skb_buf->protocol = htons(ETH_P_IP);
        new_skb_buf->dev = skb->dev;

				
		skb_reset_mac_header(new_skb_buf);
        skb_pull(new_skb_buf, ETH_HLEN);
		skb_reset_network_header(new_skb_buf);
			
		memcpy(new_skb_buf->data, skb->data, skb->len);

		
		eth = (struct ethhdr*)eth_hdr(new_skb_buf);
		eth->h_proto = htons(ETH_P_IP);

		SENDSKB(new_skb_buf);
	
		return NF_DROP;
	}
	
	return NF_ACCEPT; 
}

//handle upside packets
unsigned int stream_up_hook(unsigned int hooknum,
	struct sk_buff* skb,
	const struct net_device* in,
	const struct net_device* out,
	int(*okfn)(struct sk_buff*))
{
	PSESSIONTUPLES session_tuple;
	HASH_DATA_UP up;
	unsigned short mtu = 1280;

	//printk("stream_up_hook started\n");	
	memset(&up, 0, sizeof(HASH_DATA_UP));

	if (ipdst_filter(skb) == NF_ACCEPT)
	{
		//Log(LOG_LEVEL_NORMAL, "ipdst filter accpet");
		return NF_ACCEPT;
	}
#if 1
	if (stream_len_check(skb) < 0)
	{
		Log(LOG_LEVEL_NORMAL, "stream len check accpet");
		return NF_DROP;
	}
#endif
	if (get_hash_data_up(skb, &up) < 0)
	{
		Log(LOG_LEVEL_ERROR, "get hash data up droped");
		return NF_DROP;
	}
	
	sessionlock(&hashlock);

	session_tuple = search_nat_hashtable(skb, &up);
	if (session_tuple == NULL)
	{
		sessionunlock(&hashlock);
		Log(LOG_LEVEL_ERROR, "session tuple not found");
		return NF_DROP;
	}

	if (encap(skb, session_tuple, mtu) == -1)
	{
		sessionunlock(&hashlock);
		Log(LOG_LEVEL_ERROR, "encap failed droped");
		return NF_DROP;
	}

	sessionunlock(&hashlock);

	dev_kfree_skb(skb);
	return NF_STOLEN;
}

unsigned int stream_cfg_hook(unsigned int hooknum,
	struct sk_buff* skb,
	const struct net_device* in,
	const struct net_device* out,
	int(*okfn)(struct sk_buff*))
{

	int iret = 0;
	iret = dhcpv6_tx_captrue(skb);
	if (iret == -1 || iret == 1)
		return NF_DROP;

	return NF_ACCEPT;
}

