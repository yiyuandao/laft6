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


#define MAX_MSGSIZE 			1024

struct
{ 
	__u32 pid; 
}user_proc;

static int pcp_pid = 0;
static int icmp_pid = 0; // user pid
static int dhcp6_pid = 0; // user pid
static struct sock *nl_pcp_sk = NULL; // pcp socket
struct sock *async_nl_sk = NULL; // icmp error message socket
struct sock *dhcp6_nl_sk = NULL; // dhcpv6 reply socket
extern struct net init_net;

// laft information
LAFT_NAT_INFO laft_info = {
				.tcpport = NULL, 
				.udpport = NULL, 
				.tcp_port_used = 0, 
				.udp_port_used = 0
};

int get_icmp_error_pid(void)
{
	return icmp_pid;
}

int get_dhcp_pid(void)
{
	return dhcp6_pid;
}

/** 
 * @fn int send_nlmessage(char *message, int msglen)
 * @brief send netlink message
 * 
 * @param[in] nl_sk netlink socket
 * @param[in] pid user id
 * @param[in] message netlink message
 * @param[in] msglen  lengeth of message
 * @retval void
 */
void send_nlmsg(struct sock * nl_sk, int pid, char *message, int msglen)
{
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	int len = NLMSG_SPACE(MAX_MSGSIZE);

	if (!message || !nl_sk)
	{
		return ;
	}

	skb = alloc_skb(len, GFP_ATOMIC);
	if (skb == NULL)
	{
		Log(LOG_LEVEL_ERROR, "my_net_link:alloc_skb error\n");
		return;
	}

	nlh = nlmsg_put(skb, 0, 0, 0, MAX_MSGSIZE, 0);
	nlh->nlmsg_len = skb->len; 
	NETLINK_CB(skb).pid = 0;
	NETLINK_CB(skb).dst_group = 0;

	memcpy(NLMSG_DATA(nlh), message, msglen);

	netlink_unicast(nl_sk, skb, pid, MSG_DONTWAIT);
}

/** 
 * @fn void nl_pcp_data_ready(struct sk_buff *__skb)
 * @brief netlink callback function, get nat info
 * 
 * @param[in] __skb netlink message buf
 * @retval void
 */
void nl_pcp_data_ready (struct sk_buff* __skb)
{
	PLAFT_NAT_INFO p = NULL;
	struct sk_buff *skb = NULL;
	int nPort = 0;
	struct nlmsghdr *nlh = NULL;

	skb = skb_get(__skb);
	if(skb->len >= sizeof(struct nlmsghdr))
	{
		printk("pcp data recv start....\n");
		nlh = (struct nlmsghdr*)skb->data;

		if((nlh->nlmsg_len >= sizeof(struct nlmsghdr)) && (__skb->len >= nlh->nlmsg_len))
		{
			user_proc.pid = nlh->nlmsg_pid;
			p = (PLAFT_NAT_INFO)NLMSG_DATA(nlh);
			int i = 0;
			char *q = (char *)p;

			while (i < skb->len)
			{
				printk("%02x ", q[i++]);
			}
			printk("%02x\n", q[i++]);

			memcpy(laft_info.PubIP, p->PubIP, 4);
			printk("Pub Ip Is :%x.%x.%x.%x\n",laft_info.PubIP[0], laft_info.PubIP[1], laft_info.PubIP[2], laft_info.PubIP[3]);
			//modify by dbgong@ Tue Jul  3 16:07:41 CST 2012

			memcpy(&laft_info.LowPort, &p->LowPort, 2);
			printk("Low Port Is:%u\n",laft_info.LowPort);

			memcpy(&laft_info.HighPort, &p->HighPort, 2);
			printk("High Port Is:%u\n",laft_info.HighPort);
			printk("user_pid:%u\n",user_proc.pid);

			memcpy(laft_info.IPv6LocalAddr, p->IPv6LocalAddr, 16);
			memcpy(laft_info.IPv6RemoteAddr, p->IPv6RemoteAddr, 16);

			printk("user_pid:%u\n",user_proc.pid);

			// free tcpPort and udpPort
			kfree(laft_info.tcpport);
			kfree(laft_info.udpport);

			// add by dbgong at 2012-3-26 18:46
			// alloc mem for tcpPort and udpPort
			// nPort = portSize/8 + 1, byte equal 8 bits
			nPort = ((laft_info.HighPort - laft_info.LowPort + 1) >> 3) + 1;
			laft_info.tcpport = kmalloc(nPort, GFP_ATOMIC);
			if (laft_info.tcpport != NULL)
			{
				memset(laft_info.tcpport, 0, nPort);
				laft_info.tcp_port_used = 0;
				laft_info.udpport = kmalloc(nPort, GFP_ATOMIC);
				if (laft_info.udpport != NULL)
				{
					memset(laft_info.udpport, 0, nPort);
					laft_info.udp_port_used = 0;
				}
				else
				{
					printk("NO mem0 to alloc\n");
					kfree(laft_info.tcpport);
				}
			}
			else
			{
				printk("NO mem1 to alloc\n");
			}

		}
	}
	kfree_skb(skb);
}

/** 
 * @fn void nl_icmp_data_ready(struct sk_buff *__skb)
 * @brief netlink callback function, get the user pid
 * 
 * @param[in] __skb netlink message buf
 * @retval void
 */
static void nl_icmp_data_ready(struct sk_buff *__skb)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;

	Log(LOG_LEVEL_NORMAL, "netlink icmp socket init ok\n");

	skb = skb_get (__skb);
	if (skb->len >= NLMSG_SPACE(0))
	{
		nlh = nlmsg_hdr(skb);

		icmp_pid = nlh->nlmsg_pid;
	}
	kfree_skb(skb);
}

/** 
 * @fn void dhcp6_data_ready(struct sk_buff *__skb)
 * @brief netlink callback function, get the user pid
 * 
 * @param[in] __skb netlink message buf
 * @retval void
 */
static void dhcp6_data_ready(struct sk_buff *__skb)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;

	Log(LOG_LEVEL_NORMAL, "netlink dhcp socket init ok\n");

	skb = skb_get (__skb);
	if (skb->len >= NLMSG_SPACE(0))
	{
		nlh = nlmsg_hdr(skb);

		dhcp6_pid = nlh->nlmsg_pid;
	}
	kfree_skb(skb);
}

/** 
 * @fn Netlink_kernel_create
 * @brief wrapp netlink_kernel_create()
 * 
 * @retval void
 */
void Netlink_kernel_create(void)
{

	dhcp6_nl_sk = netlink_kernel_create(&init_net, NETLINK_DHCP6, 1,
			dhcp6_data_ready, NULL, THIS_MODULE);
	if(dhcp6_nl_sk == NULL)
	{
		Log(LOG_LEVEL_ERROR, "my_net_link: create dhcp netlink socket error.");
	}

	nl_pcp_sk = netlink_kernel_create(&init_net, NETLINK_PCP, 1,
			nl_pcp_data_ready, NULL, THIS_MODULE);
	if(nl_pcp_sk == NULL)
	{
		Log(LOG_LEVEL_ERROR, "my_net_link: create pcp netlink socket error.");
	}

	async_nl_sk = netlink_kernel_create(&init_net, NETLINK_ICMP_ERROR, 1,
			nl_icmp_data_ready, NULL, THIS_MODULE);
	if(async_nl_sk == NULL)
	{
		Log(LOG_LEVEL_ERROR, "my_async_net_link: create icmp netlink socket error.");
	}
}

/** 
 * @fn Netlink_kernel_release
 * @brief wrapp netlink_kernel_release()
 * 
 * @retval void
 */
void Netlink_kernel_release(void)
{
	netlink_kernel_release(nl_pcp_sk);
	netlink_kernel_release(async_nl_sk);
	netlink_kernel_release(dhcp6_nl_sk);
}
