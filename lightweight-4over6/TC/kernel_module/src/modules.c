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

#include <linux/netlink.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <net/net_namespace.h>
#include <net/sock.h>
#include <asm/byteorder.h>
#include "modules.h"
#include "global.h"
#include "storage.h"
#include "packet.h"
#include "log.h"
#include "defrag6.h"
#include "tccorestat.h"
/** 
* @fn  int modules_init(void)
* @brief Init modules variable,data structure.eg
* 
* @param[in] void
* 
* @retval  1 		Success 
* @retval -1		Failure 
*/
int modules_init(void)
{
	int iErr = 0;
	memset(&GlobalCtx, 0, sizeof(GlobalCtx));
#if TC_STAT_DEF 				
	InitTCCoreStatInfo();
#endif
	//init storage
	GlobalCtx.pStroageCtx = InitStorage();
	if (GlobalCtx.pStroageCtx == NULL)
	{
		Log(LOG_LEVEL_ERROR, "InitStorage error!");
		iErr = 1;
		goto ERR;
	}
	//init netlink
	Netlink_kernel_create();

	spin_lock_init(&GlobalCtx.StorageLock);
	
	//init storage timer

	init_timer(&GlobalCtx.StorageTimer);
	GlobalCtx.StorageTimer.data = 0;
	GlobalCtx.StorageTimer.function = TunnelTimeOut;
	

	initfragmodule();
	
ERR:
	if (iErr)
	{
		DestructorStorage(GlobalCtx.pStroageCtx);

		return -1;
	}
	
	return 1;
	
}

/** 
* @fn  int modules_uninit(void)
* @brief Uninit modules variable,data structure.eg
* 
* @param[in] void
* 
* @retval  1 		Success 
* @retval -1		Failure 
*/
int modules_uninit(void)
{
	unfragmodule();
	
	DestructorStorage(GlobalCtx.pStroageCtx);

	Netlink_kernel_release();
	

	return 1;
}


/** 
* @fn  unsigned int stream_hook(unsigned int hooknum,
	struct sk_buff* skb,
	const struct net_device* in,
	const struct net_device* out,
	int(*okfn)(struct sk_buff*))
* @brief Upward flow operation
* 
* @param[in] void
* 
*/
unsigned int stream_hook(unsigned int hooknum,
	struct sk_buff* skb,
	const struct net_device* in,
	const struct net_device* out,
	int(*okfn)(struct sk_buff*))
{
	int iRet = 0;

	if (GlobalCtx.uiTunState)
	{	
		iRet = Handle_skbuf(skb);
	
		if (iRet == 1 || iRet == -1)
			return NF_DROP;
	}
	
	return NF_ACCEPT;
}
