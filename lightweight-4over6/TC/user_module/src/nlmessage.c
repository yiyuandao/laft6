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
#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include <arpa/inet.h>
#include "config.h"
#include "netlinkmsgdef.h"
#include "nlmessage.h"
#include "netlink_user.h"
#include "global.h"
#include "addrpoolentity.h"
#include "log.h"

/** 
 * @fn ConstructNetlinkMsg
 * @brief construct normal netlink NETLINK_INT message:
 * (NETLINK_DETECT_MSG, NETLINK_DELETE_NODE) 
 * 
 * @param[in] msg_type message type 
 * @param[in] cliIpv6Addr client ipv6 address
 * @param[out] buf netlink message
 * @retval void
 */
void ConstructNetlinkMsg(char *buf,
									int msg_type,
									unsigned char *cliIpv6Addr, 
									unsigned long lefttime)
{
	PNETLINK_MSG_INIT pmsg = (PNETLINK_MSG_INIT)buf;
	pmsg->msghdr.msgdef = msg_type;
	memcpy(pmsg->cli_addr, cliIpv6Addr, 16);
	pmsg->lefttime = lefttime;
}

/** 
 * @fn ConstructAllocMsg
 * @brief construct netlink NETLINK_TUNNEL_MSG message(NETLINK_ALLOC_ADDR)
 * 
 * @param[in] cliIpv6Addr client ipv6 address
 * @param[in] type message type 
 * @param[in] ip external ipv4 address
 * @param[in] StartPort start port
 * @param[in] EndPort  end port
 * @param[out] buf netlink message
 * @param[in] lefttime live time
 * @retval void
 */   
void ConstructAllocMsg(char *buf, 
		unsigned char *cliIpv6Addr,
		unsigned long ip,
		unsigned short StartPort, 
		unsigned short EndPort, 
		unsigned long 	lefttime)
{
	PNETLINK_MSG_TUNCONFIG pmsg = (PNETLINK_MSG_TUNCONFIG)buf;
	pmsg->msghdr.msgdef = NETLINK_ALLOC_ADDR;
	memcpy(pmsg->user_addr, cliIpv6Addr, 16);
	pmsg->public_ip = ip;
	pmsg->start_port = StartPort;
	pmsg->end_port = EndPort;
	pmsg->lefttime = lefttime;
}

void ConstructTcStateQuery(char *buf)
{
	PTCSTATEQUERY pmsg = (PTCSTATEQUERY)buf;
	pmsg->msghdr.msgdef = NETLINK_TC_STATE;
}

void ConstructUninitMsg(PNETLINK_MSG_TCCONFIG pmsgconfig)
{
	pmsgconfig->msghdr.msgdef = NETLINK_UNINIT_CONFIG;
	pmsgconfig->uiTunState = 0;
}
/** 
* @fn HandlMessage
* @brief handle netlink message
* 
* @param[in] sock_fd netlink socket
* @param[in] msg netlink message
* @param[out] uiPublicIP external ipv4 address
* @param[out] usStartPort startport
* @retval 0 success
* @retval -1 fail
*/
int HandlMessage(int sock_fd, char *msg, char *MsgContent)
{
	PNETLINK_MSG_HDR msghdr = (PNETLINK_MSG_HDR)msg;
	char send_buf[200] = {0};
	int iRet = 0;
	
	MSGDEBUG msgdebug[NETLINK_MAX] = {	
	NETLINK_RESULT, "Return result message",
	NETLINK_DELETE_NODE, "Delete node message",
	NETLINK_ALLOC_ADDR,	"Allocate address and port",
	NETLINK_DETECT_MSG, "Detect message",		 
	NETLINK_UPDATE_MSG,	"Update message",	
	NETLINK_INIT_CONFIG, "Init config message",
	NETLINK_CRASH_MSG, "Crash Message", 
	NETLINK_TC_STATE, "TC State Message"}; 
	
	Log(LOG_LEVEL_NORMAL, "%s", msgdebug[msghdr->msgdef].msgcontent);
	switch (msghdr->msgdef)
	{
		case NETLINK_ALLOC_ADDR:
		{
			PNETLINK_MSG_INIT pmsg = (PNETLINK_MSG_INIT)msg;
			PNETLINK_MSG_TUNCONFIG ptcconfig = (PNETLINK_MSG_TUNCONFIG)MsgContent;
			
			if (ptcconfig != NULL)
			{
				// do alloc addr
				if (AllocAddrPort(&GlobalCtx.AddrPoolList,
							(unsigned short)(~GlobalCtx.Config.usPortMask + 1) / MINI_RANGE_VALUE, 
							&ptcconfig->start_port,
							&ptcconfig->end_port,
							&ptcconfig->public_ip) < 0)
				{
					Log(LOG_LEVEL_ERROR, "NETLINK_ALLOC_ADDR !!!!!!!!!!!!!!can not allocate add");
					return MSG_ERR_CANNOT_ALLOC_PORT;
				}
				else 
				{
					Log(LOG_LEVEL_NORMAL, "Alloc IP:%x, StartPort:%d, EndPort:%d",
							ptcconfig->public_ip,
							ptcconfig->start_port,
							ptcconfig->end_port);
				}
				ptcconfig->lefttime = pmsg->lefttime;
				memcpy(ptcconfig->user_addr, pmsg->cli_addr, 16);
			}
			break;
		}
		case NETLINK_UPDATE_MSG:
		{
			PNETLINK_MSG_TUNCONFIG pTunMsg = (PNETLINK_MSG_TUNCONFIG)msg;
			PNETLINK_MSG_TUNCONFIG ptcconfig = (PNETLINK_MSG_TUNCONFIG)MsgContent;

			if (ptcconfig != NULL)
			{
				memcpy(ptcconfig, pTunMsg, sizeof(NETLINK_MSG_TUNCONFIG));
			}
			break;
		}
		case NETLINK_DELETE_NODE:
		{
			unsigned int ip = 0; 
			unsigned short start_port = 0; 
			unsigned short end_port = 0; 
			PNETADDRINFO paddrinfo = (PNETADDRINFO)MsgContent;
			PNETLINK_MSG_TUNCONFIG pTunMsg = (PNETLINK_MSG_TUNCONFIG)msg;
			paddrinfo->extip = ip = pTunMsg->public_ip;
			paddrinfo->startport = start_port = pTunMsg->start_port;
			end_port = start_port +  (unsigned short)(~GlobalCtx.Config.AddrPool.usPortMask);
			paddrinfo->endport = end_port;
			// recycle addr
			if (DeallocAddrPort(&GlobalCtx.AddrPoolList,
				(unsigned short)(~GlobalCtx.Config.usPortMask + 1) / MINI_RANGE_VALUE,
				start_port,
				end_port,
				ip) < 0)
			{
				Log(LOG_LEVEL_ERROR, "NETLINK_DELETE_NODE !!!!!!!!!!!!!!can not allocate add");
				return MSG_ERR_RECYCLED_PORT;
			}
			// syslog recycle user info
			{
				int ipv4 = htonl(paddrinfo->extip);
				char log[200] = {0};
				char client_v6[60] = {0};
				char ip[16] = {0};

				sprintf(log, "- LAFT6:UserbasedW [- - %s %s - %d %d]",
						inet_ntop(AF_INET6, pTunMsg->user_addr, client_v6, sizeof(client_v6)),
						inet_ntop(AF_INET, &ipv4, ip, sizeof(ip)),
						//tunconfig.public_ip,
						paddrinfo->startport,
						paddrinfo->endport);
				syslog(LOG_INFO | LOG_LOCAL3, "%s\n", log);
			}

			break;
		}
		case NETLINK_RESULT:
		{
			PNETLINK_MSG_INIT pmsg = (PNETLINK_MSG_INIT)msg;
			if (pmsg->ucResult == 2)
			{
				Log(LOG_LEVEL_ERROR, "type: ack msg: %s", "recv ack msg from kernel");
				return MSG_ERR_RESULT;
			}
			Log(LOG_LEVEL_NORMAL, "type: ack msg: %s", "recv ack msg from kernel");
			break;
		}
		case NETLINK_TC_STATE:
		{
			PTCSTATERSP pmsg = (PTCSTATERSP)msg;
			memcpy(MsgContent, &pmsg->content[0], pmsg->msglen);
			Log(LOG_LEVEL_NORMAL,
				"State Message Content Len :%d",
				pmsg->msglen);
		}
		break;
#if 0
		case NETLINK_TC_USER_INFO:
		{
			PTCSTATERSP pmsg = (PTCSTATERSP)msg;
			memcpy(MsgContent, &pmsg->content[0], pmsg->msglen);
			Log(LOG_LEVEL_NORMAL,
				"recv netlink tc user info");
		}
		break;
#endif
		default:
			return MSG_ERR_UNKNOWN;
			break;
	}

	return MSG_ERR_SUCCESS;
}
