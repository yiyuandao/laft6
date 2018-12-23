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
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <errno.h>
#include "log.h"
#include "nlmessage.h"
#include "netlink_user.h"

#define MAX_PAYLOAD 1514 // maximum payload size

/** 
 * @fn  CloseNetLinkSocket
 * @brief close netlink socket
 * 
 * @param[in] sock_fd netlink socket
 * @retval void
 */
void CloseNetLinkSocket(int sock_fd)
{
	close(sock_fd);
}

/** 
 * @fn  InitNetLinkSocket
 * @brief create socket and bind
 * 
 * @param[in] protocol netlink protocol
 * @param[out] nl_pid netlink pid for each thread
 * @retval 0 success
 * @retval -1 failure
 */
int InitNetLinkSocket(int protocol, int *nl_pid)
{
	struct sockaddr_nl src_addr;
	int retval = 0;
	int sock_fd = 0;

	sock_fd = socket(AF_NETLINK, SOCK_RAW, protocol);
	if (sock_fd == -1)
	{
		Log(LOG_LEVEL_ERROR, "error getting socket: %s", strerror(errno));
		return -1;
	}

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	*nl_pid = src_addr.nl_pid = pthread_self() << 16 | getpid(); // self pid
	src_addr.nl_groups = 0; // multi cast

	retval = bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));
	if (retval == 0)
	{
		return sock_fd;
	}
	else
	{
		Log(LOG_LEVEL_ERROR, "bind failed: %s", strerror(errno));
		close(sock_fd);
		return -1;
	}

}

/** 
 * @fn  SendMessage
 * @brief send netlink msg
 * 
 * @param[in] sock_fd netlink socket
 * @param[in] message netlink message
 * @param[in] len  message length
 * @retval 0 success
 * @retval -1 failure
 */
int SendMessage(int sock_fd, char *message, int len, int nl_pid)
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
		close(sock_fd);
		return -1;
	}

	nlh->nlmsg_len = NLMSG_SPACE(len);
	nlh->nlmsg_pid = nl_pid;
	nlh->nlmsg_flags = 0;
	memcpy(NLMSG_DATA(nlh),message, len);

	memset(&dest_addr,0,sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0;
	dest_addr.nl_groups = 0;

	iov.iov_base = (void *)nlh;
	iov.iov_len = NLMSG_SPACE(len);

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&dest_addr;
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	state_smg = sendmsg(sock_fd, &msg, 0);
	if (state_smg == -1)
	{
		Log(LOG_LEVEL_ERROR, "get error sendmsg = %s",strerror(errno));
		return -1;
	}

	free(nlh);
	nlh = NULL;

	return 0;
}

/** 
 * @fn  RecvMessage
 * @brief receive netlink message and handle message
 * 
 * @param[in] sock_fd netlink socket
 * @param[out] uiPublicIP external ipv4 address
 * @param[out] usStartPort start port
 * @retval 0 success
 * @retval -1 failure
 */
int RecvMessage(int sock_fd, char *MsgContent, unsigned int msglen)
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
		close(sock_fd);
		return -1;
	}

	nlh->nlmsg_len = NLMSG_SPACE(msglen);
	nlh->nlmsg_pid = 0;
	nlh->nlmsg_flags = 0;
	iov.iov_base = (void *)nlh;
	iov.iov_len = NLMSG_SPACE(msglen);

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&cli_addr;
	msg.msg_namelen = sizeof(cli_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	nrecv = recvmsg(sock_fd, &msg, 0);
	if (nrecv > 0)
	{
#if 0
		if (nrecv > msglen)
		{
			Log(LOG_LEVEL_ERROR, "nrecv(%d) > msglen(%d)", nrecv, msglen);
			goto END;
		}
#endif
		memcpy(MsgContent, (char *)NLMSG_DATA(nlh), nrecv - sizeof(struct msghdr));

		free(nlh);
		nlh = NULL;

		return nrecv - sizeof(struct msghdr);
	}
	else if (nrecv == 0)
	{
		Log(LOG_LEVEL_NORMAL, "socket shutdown");
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

END:
	free(nlh);
	nlh = NULL;
	return -1;
}
