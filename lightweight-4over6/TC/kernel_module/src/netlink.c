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
#include <linux/slab.h>
#include <linux/net.h>
#include <net/netlink.h>
#include <net/sock.h>
#include "netlink.h"
#include "netlinkmsgdef.h"
#include "nlmessage.h"
#include "storage.h"
#include "log.h"
#include "global.h"
#include "packet.h"
#include "tccorestat.h"
#include "tcuserinfo.h"
#include "memdebug.h"

#define NETLINK_USER 			22
#define NETLINK_ASYNC 			26
#define NETLINK_TC_STATE		27
#define MAX_MSGSIZE 			1500
#define TUNNEL_LIVE_TIME 		60 * 60

static int pid = 0;
static int async_pid = 0;
static int user_info_pid = 0;
static struct sock *nl_sk = NULL;
struct sock *async_nl_sk = NULL;
struct sock *tc_state_sk = NULL;
struct sock *tc_user_info_sk = NULL;

extern struct net init_net;
/** 
 * @fn int send_nlmessage(char *message, int msglen)
 * @brief send netlink message
 * 
 * @param[in] message netlink message
 * @param[in] msglen  lengeth of message
 * @retval void
 */
void send_nlmsg(char *message, int msglen)
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
 * @fn int send_async_nlmessage(char *message, int msglen)
 * @brief send netlink message
 * 
 * @param[in] message netlink message
 * @param[in] msglen  lengeth of message
 * @retval void
 */
void send_async_nlmsg(char *message, int msglen)
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

	netlink_unicast(async_nl_sk, skb, async_pid, MSG_DONTWAIT);
}

void send_tc_info_nlmsg(char *message, int msglen)
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

	netlink_unicast(tc_user_info_sk, skb, user_info_pid, MSG_DONTWAIT);
}
/** 
 * @fn int HandlMessage(char *msg, unsigned int *uiPublicIP, unsigned short *usStartPort)
 * @brief handle netlink message
 * 
 * @param[in] msg netlink message
 * @param[out] uiPublicIP external ipv4 address	
 * @param[out] usStartPort startport
 * @retval 0 success
 * @retval -1 fail
 */
static int HandlMessage(char *msg)
{
	PNETLINK_MSG_HDR pmsghdr = (PNETLINK_MSG_HDR)msg;
	MSGDEBUG msgdebug[NETLINK_MAX] = {	
		NETLINK_RESULT, "Return result message",
		NETLINK_DELETE_NODE, "Delete node message",
		NETLINK_ALLOC_ADDR,	"Allocate address and port",
		NETLINK_DETECT_MSG, "Detect message",		 
		NETLINK_UPDATE_MSG,	"Update message",	
		NETLINK_INIT_CONFIG, "Init config message", 
		NETLINK_CRASH_MSG, "Crash Message", 
		NETLINK_TC_STATE, "TC State"};    	
	unsigned char send_buf[200] = {0x02};
	Log(LOG_LEVEL_NORMAL, "%s", msgdebug[pmsghdr->msgdef].msgcontent);
	switch (pmsghdr->msgdef)
	{
		case NETLINK_DETECT_MSG:
		{
			PNETLINK_MSG_INIT pnlmsg = (PNETLINK_MSG_INIT)msg;
	
			//	lookupTunnel6
			spin_lock(&GlobalCtx.StorageLock);
			PTUNNEL_CTX ptun = LookupTunnel_v6((PKEY_V6)pnlmsg->cli_addr);
			if (ptun != NULL)
			{	
				Log(LOG_LEVEL_NORMAL, "detect msg: found tunnel");
				
				ptun->ulLiveTime = pnlmsg->lefttime; // 1 hour
				
				//send update message
				ConstructTunMsg(send_buf, ptun->ucCltIPv6, NETLINK_UPDATE_MSG, 
								ptun->uiExtIP, ptun->usStartPort, ptun->usEndPort);
				spin_unlock(&GlobalCtx.StorageLock);
				send_nlmsg(send_buf, sizeof(send_buf));
			}
			else
			{
				spin_unlock(&GlobalCtx.StorageLock);
				Log(LOG_LEVEL_NORMAL, "detect msg !!!!!!!!!!!!!!!not found tunnel");
				// send alloc addr message
				ConstructIntMsg(send_buf, pnlmsg->cli_addr, NETLINK_ALLOC_ADDR, 0, pnlmsg->lefttime);
				send_nlmsg(send_buf, sizeof(send_buf));
			}
			break;
		}
		case NETLINK_ALLOC_ADDR:
			{
				PTUNNEL_CTX ptun = NULL;
				PNETLINK_MSG_TUNCONFIG ptun_msg = (PNETLINK_MSG_TUNCONFIG)msg;

				ptun = (PTUNNEL_CTX)kmalloc(sizeof(*ptun), GFP_ATOMIC);
				if (ptun == NULL)
				{
					Log(LOG_LEVEL_ERROR, "no mem");
					ConstructIntMsg(send_buf, ptun_msg->user_addr, NETLINK_RESULT, 1, ptun_msg->lefttime);
					send_nlmsg(send_buf, sizeof(send_buf));
					return -1;
				}

				memset(ptun, 0, sizeof(*ptun));
				ptun->usStartPort = ptun_msg->start_port;
				ptun->usEndPort = ptun_msg->end_port;
				ptun->uiExtIP = ptun_msg->public_ip;
				ptun->ulLiveTime = ptun_msg->lefttime;
				memcpy(ptun->ucCltIPv6, ptun_msg->user_addr, 16);

				// do insert tunnel tree
				if (CreateTunnel(ptun) < 0)
				{
					Log(LOG_LEVEL_ERROR, "create tunnel failed");
					ConstructIntMsg(send_buf, ptun_msg->user_addr, NETLINK_RESULT, 1, ptun_msg->lefttime);
					send_nlmsg(send_buf, sizeof(send_buf));
					return -1;
				}
				
				Log(LOG_LEVEL_NORMAL, "create tunnel success: count %d", SizeOfElement(GlobalCtx.pStroageCtx));
#if TC_STAT_DEF 				
				SetTCCoreStatInfo(USER_SIZE, 1);
#endif
				// send ack
				ConstructIntMsg(send_buf, ptun_msg->user_addr, NETLINK_RESULT, 0, ptun_msg->lefttime);
				send_nlmsg(send_buf, sizeof(send_buf));
				break;
			}
		case NETLINK_DELETE_NODE:
			{
				int ret = 1;
				PNETLINK_MSG_INIT pnlmsg = (PNETLINK_MSG_INIT)msg;
				unsigned int uiPublicIP = 0;
				unsigned short usStartPort = 0;
				
				// lookup tunnel, then delete tunnel
				ret = DeleteTunnel(pnlmsg->cli_addr, &usStartPort, &uiPublicIP);
				if (ret == 1)
				{
#if TC_STAT_DEF 				
					DelTCCoreStatInfo(USER_SIZE, 1);
#endif
					Log(LOG_LEVEL_NORMAL, "delete tunnel success: count %d", SizeOfElement(GlobalCtx.pStroageCtx));
					ConstructTunMsg(send_buf, pnlmsg->cli_addr, NETLINK_DELETE_NODE, 
								uiPublicIP, usStartPort, 0);
					send_nlmsg(send_buf, sizeof(send_buf));
					return 0;
				}
				else
				{
					Log(LOG_LEVEL_ERROR, "delete tunnel failed");
					ConstructIntMsg(send_buf, pnlmsg->cli_addr, NETLINK_RESULT, 1, pnlmsg->lefttime);
					send_nlmsg(send_buf, sizeof(send_buf));
					return -1;
				}
				break;
			}
		case NETLINK_TC_STATE:
			{
#if TC_STAT_DEF
				TCCORESTATINFO_T tcstatinfo;
				unsigned int len = 0;
				unsigned char *pmsg = NULL;
				memset(&tcstatinfo, 0, sizeof(tcstatinfo));
				CopyTCCoreStatInfo(&tcstatinfo);
				pmsg = ConstructTcStateMsg(&tcstatinfo, &len);
				if (pmsg)
				{
					send_nlmsg(pmsg, len);
					dbg_free_mem(pmsg);
				}
#endif
			}
		break;
#if 0
		case NETLINK_TC_USER_INFO:
			{
				process_user_info();
			}
		break;
#endif
		default:
			{
				unsigned char ipv6[16] = {0};
				ConstructIntMsg(send_buf, ipv6, NETLINK_RESULT, 1, 0);
				send_nlmsg(send_buf, sizeof(send_buf));
				return -1;
			}
			break;
	}
	return 0;
}

/** 
 * @fn void nl_data_ready(struct sk_buff *__skb)
 * @brief netlink callback function, handle the netlink message
 * 
 * @param[in] __skb netlink message buf
 * @retval void
 */
void nl_data_ready(struct sk_buff *__skb)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;

	skb = skb_get (__skb);
	if (skb->len >= NLMSG_SPACE(0))
	{
		nlh = nlmsg_hdr(skb);

		pid = nlh->nlmsg_pid;
		HandlMessage(NLMSG_DATA(nlh));	
	}
	kfree_skb(skb);
}

static void HandlAsyncMessage(char *msg)
{
	
	PNETLINK_MSG_HDR phdr = (PNETLINK_MSG_HDR)msg;
	
	switch (phdr->msgdef)
	{
	case NETLINK_INIT_CONFIG:
		{
			PNETLINK_MSG_TCCONFIG ptcconfig = (PNETLINK_MSG_TCCONFIG)msg;
			if (GlobalCtx.uiTunState == 1)
			{
				GlobalCtx.uiTunState = 0;
				del_timer(&GlobalCtx.StorageTimer);
				DestroyStorage(GlobalCtx.pStroageCtx);
			}
			Log(LOG_LEVEL_NORMAL, "handle config message");
			GlobalCtx.uiVersion = ptcconfig->uiversion;
			GlobalCtx.usMtu = ptcconfig->usmtu;
			GlobalCtx.uiExtBeginIP = ptcconfig->uibeginIP;
			GlobalCtx.uiExtEndIP = ptcconfig->uiEndIP;
			GlobalCtx.usPortRange = ptcconfig->usPortRange;
			GlobalCtx.uiOtherExtBeginIP = ptcconfig->uiOtherBeginIP;
			GlobalCtx.uiOtherExtEndIP = ptcconfig->uiOtherEndIP;
			GlobalCtx.uiMaxLifeTime = ptcconfig->uiMaxLifeTime;
			memcpy(GlobalCtx.ucTcIPv6, ptcconfig->tc_addr, 16);	
			
			GlobalCtx.StorageTimer.expires = jiffies + (GlobalCtx.uiMaxLifeTime / 4) * HZ;
			add_timer(&GlobalCtx.StorageTimer);
			GlobalCtx.uiTunState = ptcconfig->uiTunState;
		}	
		break;
	case NETLINK_UNINIT_CONFIG:
		{
			PNETLINK_MSG_TCCONFIG ptcconfig = (PNETLINK_MSG_TCCONFIG)msg;
			Log(LOG_LEVEL_NORMAL, "handle uninit config message");

			if (ptcconfig->uiTunState == 0)
			{
				GlobalCtx.uiTunState = ptcconfig->uiTunState;
				del_timer(&GlobalCtx.StorageTimer);
				DestroyStorage(GlobalCtx.pStroageCtx);
			}
		}
		break;
	default :
		break;
	}	
}

/** 
 * @fn void async_nl_data_ready(struct sk_buff *__skb)
 * @brief handle NETLINK_INIT_CONFIG msg
 * 
 * @param[in] __skb netlink message buf
 * @retval void
 */
void async_nl_data_ready(struct sk_buff *__skb)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;

	skb = skb_get (__skb);
	if (skb->len >= NLMSG_SPACE(0))
	{
		nlh = nlmsg_hdr(skb);

		async_pid = nlh->nlmsg_pid;
		HandlAsyncMessage((char *)NLMSG_DATA(nlh));	
	}
	kfree_skb(skb);
}

void tc_user_data_ready(struct sk_buff *__skb)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;

	skb = skb_get (__skb);
	if (skb->len >= NLMSG_SPACE(0))
	{
		nlh = nlmsg_hdr(skb);

		user_info_pid = nlh->nlmsg_pid;
		process_user_info((char *)NLMSG_DATA(nlh));
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

	nl_sk = netlink_kernel_create(&init_net, NETLINK_TEST, 1,
			nl_data_ready, NULL, THIS_MODULE);
	if(nl_sk == NULL)
	{
		Log(LOG_LEVEL_ERROR, "my_net_link: create netlink socket error.");
	}

	async_nl_sk = netlink_kernel_create(&init_net, NETLINK_ASYNC, 1,
			async_nl_data_ready, NULL, THIS_MODULE);
	if(async_nl_sk == NULL)
	{
		Log(LOG_LEVEL_ERROR, "my_async_net_link: create netlink socket error.");
	}

	tc_user_info_sk = netlink_kernel_create(&init_net, NETLINK_USER, 1,
			tc_user_data_ready, NULL, THIS_MODULE);
	if(tc_user_info_sk == NULL)
	{
		Log(LOG_LEVEL_ERROR, "my_tc_user_info_link: create netlink socket error.");
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
	//sock_release(nl_sk->sk_socket);
	//sock_release(async_nl_sk->sk_socket);
	netlink_kernel_release(nl_sk);
	netlink_kernel_release(async_nl_sk);
	netlink_kernel_release(tc_user_info_sk);
}
