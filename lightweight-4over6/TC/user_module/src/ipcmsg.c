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
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <arpa/inet.h>
#include "ipcmsgdef.h"
#include "ipcsrv.h"
#include "tcuserinfo.h"
#include "netlink_user.h"
#include "netlinkmsgdef.h"
#include "nlmessage.h"
#include "log.h"
#include "global.h"
#include "ipcmsg.h"
#include "tcuserinfo.h"


typedef struct _tag_link
{
	struct _tag_link *pnext;
	USER_MAP_INFO_ADD_RESULT result;
}RESULT_USER_NODE, *PRESULT_USER_NODE;


static void DumpTcUserInfo(PUSERINFO pinfo, int nuser)
{
	PUSERINFO puser = pinfo;
	int i = 0;
	int j = 0;

	//Log(LOG_LEVEL_NORMAL, "++++++++++++++++recved: %d", recved);
	printf("num     IPv6                                     IP           start_port   end_port\n");

	while (i < nuser)
	{
		fprintf(stderr, "%d\t", i + 1);

		while (j < 14)
		{
			fprintf(stderr, "%02x%02x:", puser[i].user_addr[j], puser[i].user_addr[j+1]);
			j += 2;
		}
		fprintf(stderr, "%02x%02x\t", puser[i].user_addr[j], puser[i].user_addr[j+1]);

		fprintf(stderr, "0x%x\t", puser[i].public_ip);
		fprintf(stderr, "%u\t\t", ntohs(puser[i].start_port));
		fprintf(stderr, "%u\t", ntohs(puser[i].end_port));

		fprintf(stderr, "\n");

		i++;
		j = 0;
	}
}

static void SendUserMapRsp(unsigned long usernum, unsigned long offset, PIPCTASKCTX pipctaskctx)
{
	int nleft = 0;
	int nuser = 0;
	int nsize = 0;
	int usertablelen = 0;
	PIPC_MSG_USER_MAP_RSP prsp = NULL;

	if (offset > total_user)
	{
		Log(LOG_LEVEL_ERROR, "wrong userpageoffset....");
		return;
	}

	nleft = total_user - offset;
	if (nleft >= 50)
	{
		nuser = 50;
	}
	else
	{
		nuser = nleft;
	}

	usertablelen = nuser * sizeof(USER_MAP_INFO);	
	nsize = sizeof(IPC_MSG_USER_MAP_RSP) - sizeof(unsigned char) + usertablelen;
	if ( (prsp = (PIPC_MSG_USER_MAP_RSP)malloc(nsize)) != NULL) 
	{
		prsp->ipcmsghdr.ipcmsgtype = IPC_MSG_ONLINE_USER_MAP_RSP;
		prsp->usernum = htonl(nuser);
		prsp->usertablelen = htonl(usertablelen);

		// copy map info
		{
			int i = 0;
			PUSER_MAP_INFO pmap = (PUSER_MAP_INFO)prsp->usertable;

			while (i < nuser)
			{
				pmap[i].sport = htons(UserPool[offset + i].start_port);
				pmap[i].eport = htons(UserPool[offset + i].end_port);
				pmap[i].extip = htonl(UserPool[offset + i].public_ip);
				memcpy(pmap[i].useraddr, UserPool[offset + i].user_addr, 16);
				pmap[i].lefttime = htonl(UserPool[offset + i].lefttime);
				i++;
			}
		}
		ipc_server_send((unsigned char *)prsp, nsize, 
							(struct sockaddr *)&pipctaskctx->addr,
							sizeof(pipctaskctx->addr));
	}
	else
	{
		Log(LOG_LEVEL_ERROR, "low mem....");
		return;
	}
}

int SendUserAddResultRsp(unsigned int count, PRESULT_USER_NODE pUserList,  PIPCTASKCTX pipctaskctx)
{
	PRESULT_USER_NODE pNode = NULL, pNext = NULL;
	PIPC_MSG_USER_MAP_ADD_RSP pRspMsg = NULL;
	int off = 0;
	int msglen = sizeof(IPC_MSG_USER_MAP_ADD_RSP) - sizeof(unsigned char) + count * sizeof(USER_MAP_INFO_ADD_RESULT);
	pRspMsg = (PIPC_MSG_USER_MAP_ADD_RSP)malloc(msglen);
	if (pRspMsg == NULL)
		return -1;
	pRspMsg->ipcmsghdr.ipcmsgtype = IPC_MSG_ONLINE_USER_ADD_RSP;
	pRspMsg->usernum = htonl(count);
	pRspMsg->usertablelen = htonl(sizeof(USER_MAP_INFO_ADD_RESULT) * count);
	pNode = pUserList->pnext;	
	while (pNode != NULL)
	{
		pNext = pNode->pnext;
		memcpy(&pRspMsg->usertalbe[off],
			&pNode->result, sizeof(USER_MAP_INFO_ADD_RESULT));
		free(pNode);
		pNode = NULL;
		pNode = pNext;
		off += sizeof(USER_MAP_INFO_ADD_RESULT);
	}
	
	ipc_server_send((unsigned char *)pRspMsg, msglen, 
						(struct sockaddr *)&pipctaskctx->addr,
						sizeof(pipctaskctx->addr));
	 free(pRspMsg);
	 return 1;
}
//return value
//@-1	fail
//@0	add
//@1	exist
static int GetNetLinkMsgType(unsigned char *msg)
{
	PNETLINK_MSG_HDR msghdr = (PNETLINK_MSG_HDR)msg;
	if (msghdr->msgdef == NETLINK_ALLOC_ADDR){
		return 0;	
	}else if (msghdr->msgdef == NETLINK_UPDATE_MSG) {
		return 1;
	}

	return -1;
}

//
//return value
//@0	sucess
//@1	exist
//@2	fail
//
int AddStaticTunnel(PUSER_MAP_INFO pmap)
{
	char send_buf[1500] = {0};
	char recv_buf[1500] = {0};
	int iRet = 0;
	NETLINK_MSG_TUNCONFIG tcconfig;
	int nl_pid = 0;
	int sock_fd;
	
	sock_fd = InitNetLinkSocket(NETLINK_TEST, &nl_pid);
	if (sock_fd < 0)
	{
		Log(LOG_LEVEL_ERROR, "can not create netlink socket");
		return 2;
		
	}
	//construct detect msg
	ConstructNetlinkMsg(send_buf,
			NETLINK_DETECT_MSG,
			pmap->useraddr,
			SUCCESS_LIFETIME);

	Log(LOG_LEVEL_NORMAL, "send detect msg");
	//send detect msg
	if (SendMessage(sock_fd, send_buf, sizeof(send_buf), nl_pid) == 0)
	{ 
		//recvmsg and process message
		iRet = RecvMessage(sock_fd, recv_buf, sizeof(recv_buf));
		if (iRet < 0)
		{
			CloseNetLinkSocket(sock_fd);
			return 2;
		}
		iRet = HandlMessage(sock_fd, recv_buf, NULL);
		if (iRet != MSG_ERR_SUCCESS)
		{
			CloseNetLinkSocket(sock_fd);
			return 2;
		}
		iRet = GetNetLinkMsgType(recv_buf);
		if (iRet == 1)
		{	
			CloseNetLinkSocket(sock_fd);
			Log(LOG_LEVEL_NORMAL, "The node is exist");
			return 1;
		} else if (iRet < 0) {
			CloseNetLinkSocket(sock_fd);
			Log(LOG_LEVEL_NORMAL, "unknow msg type");
			return 2;
		}
		//send alloc_addr msg
		Log(LOG_LEVEL_NORMAL, "send request allocate add");
		// construct msg
		ConstructAllocMsg(send_buf,
		pmap->useraddr,
		ntohl(pmap->extip),
		ntohs(pmap->sport),
		ntohs(pmap->eport),
		ntohl(pmap->lefttime));

		//send allocate address and port request
		iRet = SendMessage(sock_fd, send_buf, sizeof(send_buf), nl_pid);
		if (iRet < 0)
		{
			CloseNetLinkSocket(sock_fd);
			return 2;
		}
		//return result
		iRet = RecvMessage(sock_fd, recv_buf, sizeof(recv_buf));
		if (iRet < 0)
		{
			CloseNetLinkSocket(sock_fd);
			return 2;
		}
		iRet = HandlMessage(sock_fd, recv_buf, NULL);
		if (iRet != MSG_ERR_SUCCESS)
		{
			CloseNetLinkSocket(sock_fd);
			return 2;
		}
	
		// syslog user info
		 {
			 int ipv4 = htonl(pmap->extip);
			 char log[200] = {0};
			 char client_v6[60] = {0};
			 char ip[16] = {0};

			 sprintf(log, "- LAFT6:UserbasedA [- - %s %s - %d %d]",
					 inet_ntop(AF_INET6, pmap->useraddr, client_v6, sizeof(client_v6)),
					 inet_ntop(AF_INET, &ipv4, ip, sizeof(ip)),
					 //tunconfig.public_ip,
					 pmap->sport,
					 pmap->eport);
			 syslog(LOG_INFO | LOG_LOCAL3, "%s\n", log);
		 }

		CloseNetLinkSocket(sock_fd);
		return 0;
	} 
	else
	{ 
		CloseNetLinkSocket(sock_fd);
		return 2;
	} 
	CloseNetLinkSocket(sock_fd);
	return 2;
}

void ipc_msg_handle(void *msg)
{
	PIPCTASKCTX pipctaskctx = (PIPCTASKCTX)msg;
	PIPC_MSG_HDR pmsg = (PIPC_MSG_HDR)pipctaskctx->msgcontext;
	int nl_sock  = 0;
	int nl_pid  = 0;

	if ( (nl_sock = InitNetLinkSocket(22, &nl_pid)) < 0)
	{
		Log(LOG_LEVEL_ERROR, "init netlink socket failed");
		free(msg);
		return;
	}
	Log(LOG_LEVEL_NORMAL, "+++++++++before SendMessage+++socket: %d++pid: %d+++", nl_sock, nl_pid);
	switch (pmsg->ipcmsgtype)
	{
		case IPC_MSG_ONLINE_USER_NUM_REQ:
			{
				int nuser = 0;

				Log(LOG_LEVEL_NORMAL, "IPC_MSG_ONLINE_USER_NUM_REQ ");
				nuser = GetTcUserNumber(nl_sock, nl_pid);

				IPC_MSG_USER_NUM_RSP rsp;
				rsp.ipcmsghdr.ipcmsgtype = IPC_MSG_ONLINE_USER_NUM_RSP;
				rsp.usernum = htonl(nuser);
				
				Log(LOG_LEVEL_NORMAL, "IPC_MSG_ONLINE_USER_NUM_REQ usernum : %d", nuser);

				// load data
				GetTcUserData(nl_sock, nl_pid);
				recved = 0;

				ipc_server_send((unsigned char *)&rsp, sizeof(rsp), 
						(struct sockaddr *)&pipctaskctx->addr,
						sizeof(pipctaskctx->addr));
			}
			break;
		case IPC_MSG_ONLINE_USER_MAP_REQ:
			{
				PIPC_MSG_USER_MAP_REQ preq = (PIPC_MSG_USER_MAP_REQ)pipctaskctx->msgcontext;
				unsigned long usernum = ntohl(preq->userpagenum);
				unsigned long userpageoffset = ntohl(preq->userpageoffset);

				Log(LOG_LEVEL_NORMAL,
					"IPC_MSG_ONLINE_USER_MAP usernum : %ld offset %ld",
					usernum, userpageoffset);

				//GetTcUserData(nl_sock, nl_pid);
				SendUserMapRsp(usernum, userpageoffset,  pipctaskctx);
			}
			break;
		case IPC_MSG_ONLINE_USER_ADD_REQ:
			{
				PIPC_MSG_USER_MAP_ADDR_REQ preq = (PIPC_MSG_USER_MAP_ADDR_REQ)pipctaskctx->msgcontext;
				unsigned long nuser = ntohl(preq->usernum);
				unsigned long i = 0;
				unsigned long len = htonl(preq->usertablelen);
				int ret = 0;
				RESULT_USER_NODE UserListHead;
				PRESULT_USER_NODE pNode = NULL, pCur = NULL;
				int Count = 0;
				
				UserListHead.pnext = NULL;
				pCur = &UserListHead;
				Log(LOG_LEVEL_NORMAL, "IPC_MSG_ONLINE_USE_ADD_REQ nuser: %ld" , nuser);
				//DumpTcUserInfo((PUSERINFO)preq->usertable, nuser);
				while (i < len)
				{
					ret = AddStaticTunnel((PUSER_MAP_INFO)&preq->usertable[i]);
					if (ret != 0) {
						pNode = (PRESULT_USER_NODE)malloc(sizeof(RESULT_USER_NODE));
						memcpy(&pNode->result.usermapinfo,
							(PUSER_MAP_INFO)&preq->usertable[i],
							sizeof(USER_MAP_INFO));
						pNode->result.result = htonl(ret);
						pNode->pnext = NULL;
						if (UserListHead.pnext == NULL) {
							UserListHead.pnext = pNode;
						}
						else {
							pCur->pnext = pNode;
						}
						pCur = pNode;
						Count ++;
					}
					Log(LOG_LEVEL_NORMAL, "AddStaticTunnel retvalue:%d" , ret);
					i += sizeof(USER_MAP_INFO);
				}
				SendUserAddResultRsp(Count, &UserListHead, pipctaskctx);
			}
			break;
		default:
			Log(LOG_LEVEL_WARN, "unknow request");
			break;
	}

	free(msg);
	CloseNetLinkSocket(nl_sock);
}
