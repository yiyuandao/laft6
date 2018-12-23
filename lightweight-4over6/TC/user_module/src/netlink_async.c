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
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <errno.h>
#include <arpa/inet.h>

#include "log.h"
#include "config.h"
#include "global.h"
#include "netlinkmsgdef.h"
#include "nlmessage.h"
#include "addrpoolentity.h"
#include "icmperror.h"
#define NETLINK_ASYNC 26
#define MAX_PAYLOAD 256 // maximum payload size

static int async_sock_fd = 0;

/** 
 * @fn  CloseNetLinkSocket
 * @brief close netlink socket
 * 
 * @retval void
 */
void CloseAsyncNetLinkSocket(void)
{
	close(async_sock_fd);
}

/** 
 * @fn  InitNetLinkSocket
 * @brief create socket and bind
 * 
 * @param[in] None
 * @retval 0 success
 * @retval -1 failure
 */
int InitAsyncNetLinkSocket(void)
{
	struct sockaddr_nl src_addr;
	int retval = 0;

	async_sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ASYNC);
	if (async_sock_fd == -1)
	{
		Log(LOG_LEVEL_ERROR, "error getting socket: %s", strerror(errno));
		return -1;
	}

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid(); // self pid
	src_addr.nl_groups = 0; // multi cast

	retval = bind(async_sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));
	if (retval == 0)
	{
		return 0;
	}
	else
	{
		Log(LOG_LEVEL_ERROR, "bind failed: %s", strerror(errno));
		close(async_sock_fd);
		return -1;
	}
}

/** 
 * @fn  SendMessage
 * @brief send netlink msg
 * 
 * @param[in] message netlink message
 * @param[in] len  message length
 * @retval 0 success
 * @retval -1 failure
 */
int SendAsyncMessage(char *message, int len)
{
	struct sockaddr_nl dest_addr;
	struct nlmsghdr *nlh = NULL;
	struct msghdr msg;
	struct iovec iov;
	int state_smg = 0;

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
	if (nlh == NULL)
	{
		Log(LOG_LEVEL_ERROR, "malloc nlmsghdr error!");
		close(async_sock_fd);
		return -1;
	}


	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;
	memcpy(NLMSG_DATA(nlh),message, len);

	memset(&dest_addr,0,sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0;
	dest_addr.nl_groups = 0;

	iov.iov_base = (void *)nlh;
	iov.iov_len = NLMSG_SPACE(MAX_PAYLOAD);

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&dest_addr;
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	state_smg = sendmsg(async_sock_fd, &msg, 0);
	if (state_smg == -1)
	{
		Log(LOG_LEVEL_ERROR, "get error sendmsg = %s",strerror(errno));
		//exit(-1);
		return -1;
	}

	free(nlh);
	nlh = NULL;

	return 0;
}


/** 
 * @fn HandlAsyncMessage
 * @brief handle async netlink message
 * 
 * @param[in] msg netlink message
 * @retval 0 success
 * @retval -1 failure
 */
int HandlAsyncMessage(char *msg)
{
	PNETLINK_MSG_HDR pmsghdr = (PNETLINK_MSG_HDR)msg;
	unsigned short start_port = 0; 
	unsigned short end_port = 0; 

	switch(pmsghdr->msgdef)
	{
	case NETLINK_DELETE_NODE:
		{
			PNETLINK_MSG_TUNCONFIG pTunMsg = (PNETLINK_MSG_TUNCONFIG)msg;
			Log(LOG_LEVEL_NORMAL, "type: %d handle delete node msg", pTunMsg->msghdr.msgdef);
			start_port = pTunMsg->start_port;
			end_port = start_port + (~GlobalCtx.Config.AddrPool.usPortMask);

			// recycle addr
			if (DeallocAddrPort(&GlobalCtx.AddrPoolList, 
						(unsigned short)(~GlobalCtx.Config.usPortMask + 1) / MINI_RANGE_VALUE, 
						pTunMsg->start_port, end_port, pTunMsg->public_ip) < 0)
			{
				Log(LOG_LEVEL_ERROR, "NETLINK_DELETE_NODE !!!!!!!!!!!!!!can not allocate addr\n");
				return -1;
			}
			else 
				Log(LOG_LEVEL_ERROR, "NETLINK_DELETE_NODE !!!!!!!!!!!!!s");

			// syslog recycle user info
			{
				int ipv4 = htonl(pTunMsg->public_ip);
				char log[200] = {0};
				char client_v6[60] = {0};
				char ip[16] = {0};

				sprintf(log, "- LAFT6:UserbasedW [- - %s %s - %d %d]",
						inet_ntop(AF_INET6, pTunMsg->user_addr, client_v6, sizeof(client_v6)),
						inet_ntop(AF_INET, &ipv4, ip, sizeof(ip)),
						start_port,
						end_port);
				syslog(LOG_INFO | LOG_LOCAL3, "%s\n", log);
			}

		}
		break;
	case NETLINK_CRASH_MSG:
	    {
#define IP6SRC		8
#define IP6DST		24
			PNETLINK_MSG_CRASHPKT pmsg = (PNETLINK_MSG_CRASHPKT)msg;

			Icmpv6ErrorSend(GlobalCtx.Config.tc_config.ucLocalIPv6Addr,
				&pmsg->pkt[IP6SRC], 
				pmsg->pkt,
				pmsg->uiLen,
				3);
		}
		break;
	case NETLINK_ICMP_ERR_MSG:
		{
			PNETLINK_MSG_ICMPERR pmsg = (PNETLINK_MSG_ICMPERR)msg;
			struct icmphdr icmphdr_t;
			struct iphdr *piphdr = NULL;
			
			memset(&icmphdr_t, 0, sizeof(struct icmphdr));
			icmphdr_t.checksum = 0;
			icmphdr_t.type = 3;
			icmphdr_t.code = 4;
			icmphdr_t.un.frag.mtu = htons(GlobalCtx.Config.tc_config.usmtu);
			piphdr = (struct iphdr *)&pmsg->pkt[0];
			
			IcmpErrorSend(&icmphdr_t,
				htonl(piphdr->saddr),
				&pmsg->pkt[0],
				pmsg->uiLen,
				1);	
			Log(LOG_LEVEL_NORMAL, "NETLINK_ICMP_ERR_MSG");

		}
		break;
	default:
		break;
	}
	
	return 0;
}

/** 
 * @fn  RecvAsyncMessage
 * @brief receive netlink message and handle message
 * 
 * @retval 0 success
 * @retval -1 failure
 */
int RecvAsyncMessage(void)
{
	struct nlmsghdr *nlh = NULL;
	struct sockaddr_nl cli_addr;
	struct msghdr msg;
	struct iovec iov;
	int nrecv = 0;

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
	if (nlh == NULL)
	{
		Log(LOG_LEVEL_ERROR, "malloc nlmsghdr error!");
		close(async_sock_fd);
		return -1;
	}

	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;
	iov.iov_base = (void *)nlh;
	iov.iov_len = NLMSG_SPACE(MAX_PAYLOAD);

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&cli_addr;
	msg.msg_namelen = sizeof(cli_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	while (1)
	{
		nrecv = recvmsg(async_sock_fd, &msg, 0);
		if (nrecv > 0)
		{
			int ret = 0;
			// handle netlink_delete_node msg
			ret = HandlAsyncMessage((char *)NLMSG_DATA(nlh));
			
		}
		else if (nrecv == 0)
		{
			Log(LOG_LEVEL_ERROR, "socket shutdown");
			goto END;
		}
		else
		{
			if (errno == EBADF)
			{
				perror("!!!!!!!!!!!!!!recvmessage");
				goto END;
			}
		}
	}
END:
	free(nlh);
	nlh = NULL;
	return -1;
}

/** 
 * @fn  process_del_node
 * @brief handle delete node message
 * 
 * @retval void
 */
void process_del_node(void *param)
{
	if (RecvAsyncMessage() < 0)
	{
		Log(LOG_LEVEL_ERROR, "hande delete node message error");
		return;
	}
}
