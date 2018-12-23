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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/in6.h>
#include <linux/ipv6.h>
#include <linux/icmp.h>
#include <net/ipv6.h>
#include <net/udp.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <net/tcp.h>
#include <linux/string.h>
#include <linux/if_ether.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/rtc.h>
#include <linux/inet.h>
#include <linux/netlink.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <net/net_namespace.h>
#include <net/sock.h>
#include <asm/byteorder.h>
#include "memdebug.h"
#include "netlinkmsgdef.h"
#include "nlmessage.h"
#include "tccorestat.h"
/** 
 * @fn ConstructIntMsg
 * @brief construct netlink message
 * 
 * @param[in] cliIpv6Addr client ipv6 address
 * @param[in] type message type 
 * @param[in] result  0: default 1: fail
 * @param[out] buf netlink message
 * @retval void
 */
void ConstructIntMsg(char *buf, 
							unsigned char *cliIpv6Addr,
							int type, unsigned char result,
							unsigned long lefttime)
{
	PNETLINK_MSG_INIT pmsg = (PNETLINK_MSG_INIT)buf;
	pmsg->msghdr.msgdef = type;
	pmsg->ucResult = result;
	pmsg->lefttime = lefttime;
	memcpy(pmsg->cli_addr, cliIpv6Addr, 16);
}

/** 
 * @fn ConstructTunMsg
 * @brief construct netlink message for tunnel
 * 
 * @param[in] cliIpv6Addr client ipv6 address
 * @param[in] type message type 
 * @param[in] ip external ipv4 address
 * @param[in] start_port start port
 * @param[in] end_port  end port
 * @param[out] buf netlink message
 * @retval void
 */
void ConstructTunMsg(char *buf, unsigned char *cliIpv6Addr, int type, unsigned int ip, 
					unsigned short start_port, unsigned short end_port)
{
	PNETLINK_MSG_TUNCONFIG pmsg = (PNETLINK_MSG_TUNCONFIG)buf;
	pmsg->msghdr.msgdef	= type;
	pmsg->start_port = start_port;
	pmsg->end_port = end_port;
	pmsg->public_ip = ip;
	memcpy(pmsg->user_addr, cliIpv6Addr, 16);
}

/** 
 * @fn ConstructCrashPktMsg
 * @brief construct netlink message for crash message
 * 
 * @param[in] skb IPinIP packet
 * @param[out] len IPinIP packet length 
 * @retval Crash packet message pointer
 */
unsigned char* ConstructCrashPktMsg(struct sk_buff *skb, unsigned int *len)
{
	PNETLINK_MSG_CRASHPKT pmsg = NULL;
	unsigned int uilen = 0, l = 0;
	
	if (skb->len > 68)
		l = 68;
	else
		l = skb->len;
	
	uilen = sizeof(NETLINK_MSG_CRASHPKT) - 1 + l;
	
	pmsg = (PNETLINK_MSG_CRASHPKT)dbg_alloc_mem(uilen);
	if (pmsg == NULL)
		return NULL;
	
	pmsg->msghdr.msgdef = NETLINK_CRASH_MSG;
	pmsg->uiLen = l;
	
	memcpy((unsigned char *)&pmsg->pkt[0], (unsigned char *)skb->data, l);
	*len = uilen;
	return (unsigned char *)pmsg;
}

unsigned char *ConstructIcmpErrMsg(struct sk_buff *skb, unsigned int *len)
{
	PNETLINK_MSG_ICMPERR pIcmpErrMsg = NULL;
	
	unsigned int uilen = 0, l = 0;
	
	if (skb->len > 28)
		l = 28;
	else
		l = skb->len;

	uilen = sizeof(NETLINK_MSG_ICMPERR) - 1 + l;
	
	pIcmpErrMsg = (PNETLINK_MSG_ICMPERR)dbg_alloc_mem(uilen);
	if (pIcmpErrMsg == NULL)
		return NULL;
	
	pIcmpErrMsg->msghdr.msgdef = NETLINK_ICMP_ERR_MSG;
	pIcmpErrMsg->uiLen = l;
	
	memcpy((unsigned char *)&pIcmpErrMsg->pkt[0], (unsigned char *)skb->data, l);
	*len = uilen;
	return (unsigned char *)pIcmpErrMsg;
}

unsigned char *ConstructTcStateMsg(PTCCORESTATINFO_T tccorestatinfo, unsigned int *len)
{
	PTCSTATERSP pmsg = NULL;
	unsigned int uilen = 0;
	
	uilen = sizeof(TCCORESTATINFO_T) + sizeof(TCSTATERSP) - 1;
	
	pmsg = (PTCSTATERSP)dbg_alloc_mem(uilen);
	if (pmsg == NULL)
		return NULL;
	
	pmsg->msghdr.msgdef = NETLINK_TC_STATE;
	pmsg->msglen = sizeof(TCCORESTATINFO_T);
	memcpy((unsigned char *)&pmsg->content[0], (unsigned char *)tccorestatinfo, sizeof(TCCORESTATINFO_T));
	*len = uilen;
	return (unsigned char *)pmsg;
}
