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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <asm/types.h>
#include <linux/socket.h>
#include <errno.h>

#include "ipcmsgdef.h"


#define MAX_USER_NUMBER_PAGE		50

static int ipc_online_user_num_req_create(PIPC_MSG_USER_NUM_REQ pmsg)
{
	pmsg->ipcmsghdr.ipcmsgtype = IPC_MSG_ONLINE_USER_NUM_REQ;
	return sizeof(IPC_MSG_USER_NUM_REQ);
}

static int ipc_user_map_req_create(PIPC_MSG_USER_MAP_REQ pmsg, 
												unsigned int userpagenum, 
												unsigned int userpageoffset)
{
	pmsg->ipcmsghdr.ipcmsgtype = IPC_MSG_ONLINE_USER_MAP_REQ;
	pmsg->userpagenum = htonl(userpagenum);
	pmsg->userpageoffset = htonl(userpageoffset);
	return sizeof(IPC_MSG_USER_MAP_REQ);
}

static unsigned long get_tc_user_number(unsigned char *recvbuf)
{
	PIPC_MSG_USER_NUM_RSP pmsgrsp = (PIPC_MSG_USER_NUM_RSP)recvbuf;
	if (pmsgrsp->ipcmsghdr.ipcmsgtype != IPC_MSG_ONLINE_USER_NUM_RSP)
		return 0;

	return htonl(pmsgrsp->usernum);
}

void  print_tc_user_map(PIPC_MSG_USER_MAP_RSP pipcusermapmsg)
{
	int usernum = 0;
	int indx = 0;
	PUSER_MAP_INFO pmapinfo = NULL;
	char ipv6addr[128] = {0};
	char ipv4addr[64] = {0};
	static int no = 0;
	printf("Index\tUser's IPv6\t\t\t\tExtern IPv4\t\tPort Range\t\tLefttime\n\n");
	usernum = htonl(pipcusermapmsg->usernum);
	
	while (indx < htonl(pipcusermapmsg->usertablelen))
	{
		pmapinfo = (PUSER_MAP_INFO)&pipcusermapmsg->usertalbe[indx];
		inet_ntop(AF_INET6, pmapinfo->useraddr, ipv6addr, sizeof(ipv6addr));
		inet_ntop(AF_INET, &pmapinfo->extip, ipv4addr, sizeof(ipv4addr));
		printf("%d\t%s\t%s\t\t%d-%d\t\t%d\n", 
			no++, ipv6addr, ipv4addr, htons(pmapinfo->sport), htons(pmapinfo->eport), htonl(pmapinfo->lefttime));		
		indx += sizeof(USER_MAP_INFO);
	}
}

static unsigned long get_tc_user_map(unsigned char *recvbuf)
{
	PIPC_MSG_USER_MAP_RSP pipcusermapmsg = (PIPC_MSG_USER_MAP_RSP)recvbuf;
	if (pipcusermapmsg->ipcmsghdr.ipcmsgtype != IPC_MSG_ONLINE_USER_MAP_RSP)
		return 0;
	print_tc_user_map(pipcusermapmsg);
	return htonl(pipcusermapmsg->usernum);
}
int main(int argc, char *argv[])
{
	int msglen = 0, ret = 0;
	unsigned char sendbuf[512] = {0};
	unsigned char recvbuf[1500] = {0};
	struct sockaddr_in srvaddr;
	int offset = 0;
	int maxusernum = 0;

	if (argc != 2)
	{
		printf("useage: %s 127.0.0.1\n", argv[0]);
		return 0;
	}
	memset(&srvaddr, 0, sizeof(srvaddr));
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = htons(55501);
	srvaddr.sin_addr.s_addr = inet_addr(argv[1]);

	msglen = ipc_online_user_num_req_create((PIPC_MSG_USER_NUM_REQ)sendbuf);
	ret = ipc_clt_send(sendbuf,
		msglen, 
		recvbuf, 
		sizeof(recvbuf), 
		(struct sockaddr*)&srvaddr, 1);
	if (ret < 0){
		printf("ipc_clt_send error!\n");
		return -1;
	}
	maxusernum = get_tc_user_number(recvbuf);
	
	while (offset < maxusernum)
	{
		printf("The current number of online users:%d\n", maxusernum);	
		msglen = ipc_user_map_req_create((PIPC_MSG_USER_MAP_REQ)sendbuf,
			MAX_USER_NUMBER_PAGE,
			offset);
		ret = ipc_clt_send(sendbuf,
			msglen, 
			recvbuf, 
			sizeof(recvbuf), 
			(struct sockaddr*)&srvaddr, 1);
		if (ret < 0){
			return -1;
		}
		offset += get_tc_user_map(recvbuf);
		if (offset < maxusernum && offset > 0) {
			printf("\t\t\t\t--more(Press Enter)--\n");
			getc(stdin);
		}
	}
	
	return 1;
}
