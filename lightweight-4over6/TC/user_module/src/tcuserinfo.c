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
#include <stdlib.h>
#include <string.h>
#include "tcuserinfo.h" 
#include "netlink_user.h"
#include "netlinkmsgdef.h"
#include "log.h" 

#define MAX_MSG_LEN 				1500


int total_user = 0;
int recved = 0;
USERINFO UserPool[MAX_USER_POOL_NUM] = {0};

static void print_msg(unsigned char *msg, int len);
PUSERINFO AllocUserInfoPool(int nuser)
{
	PUSERINFO puser = NULL;
	int size = nuser * sizeof(*puser);

	if ( (puser = malloc(size)) != NULL)
	{
		memset(puser, 0, size);
		return puser;
	}
	else
	{
		Log(LOG_LEVEL_ERROR, "low mem... exit now ");
		exit(-1);
	}
}

static void AddUserInfoToPool(int nuser, PUSERINFO puser)
{
	//Log(LOG_LEVEL_NORMAL, "++++++++------------------++++++++bytes: %d", nuser * sizeof(*puser));
	//print_msg((unsigned char *)puser,  nuser * sizeof(*puser));
	memcpy(&UserPool[recved], puser, nuser * sizeof(USERINFO));
	recved += nuser;
}

static void DumpUserInfo(void)
{
	PUSERINFO puser = &UserPool[0];
	int i = 0;
	int j = 0;

	Log(LOG_LEVEL_NORMAL, "++++++++++++++++recved: %d", recved);
	printf("num     IPv6                                     IP           start_port   end_port\n");

	while (i < recved)
	{
		fprintf(stderr, "%d\t", i + 1);

		while (j < 14)
		{
			fprintf(stderr, "%02x%02x:", puser[i].user_addr[j], puser[i].user_addr[j+1]);
			j += 2;
		}
		fprintf(stderr, "%02x%02x\t", puser[i].user_addr[j], puser[i].user_addr[j+1]);

		fprintf(stderr, "0x%x\t", puser[i].public_ip);
		fprintf(stderr, "%u\t\t", puser[i].start_port);
		fprintf(stderr, "%u\t", puser[i].end_port);

		fprintf(stderr, "\n");

		i++;
		j = 0;
	}
}

static void BuildUserQueryMsg(char *query, int type)
{
	PTCUSERQUERY pquery = (PTCUSERQUERY)query;
	pquery->msghdr.msgdef = type;
}

static void print_msg(unsigned char *msg, int len)
{
	int i = 0;

	Log(LOG_LEVEL_NORMAL, "))))))))))))msg len: %d ", len);
	
	while (i < len - 1)
	{
		printf("%02x ", msg[i]);
		i++;
	}
	printf("%02x\n", msg[i]);
}

static int ProcessNLMessage(char *msg)
{
	PNETLINK_MSG_HDR msghdr = (PNETLINK_MSG_HDR)msg;
	PTCUSERRESP prsp = (PTCUSERRESP)msg;

	 switch (msghdr->msgdef)
	 {
		  case NETLINK_TC_USER_INFO:
		  {
			  // storage user info
			  int nuser = prsp->msglen / sizeof(USERINFO);
			  Log(LOG_LEVEL_NORMAL, "++++++++++++++++++++++nuser: %d", nuser);
			  AddUserInfoToPool(nuser, prsp->user);
			  return 1;
		  break;
		  }
#if 0
		  case NETLINK_TC_USER_TOTAL:
			  total_user = prsp->total_user;

			  Log(LOG_LEVEL_NORMAL, "++++++++++++++++++total user: %d", total_user);
			  // alloc mem to storage user info
			  pUserPool = AllocUserInfoPool(total_user);
			  return 1;
#endif
		  case NETLINK_RESULT:
			  // success;
			  //recved = 0;
			  //DumpUserInfo();
			  return 0;
			  break;
		  default:
			  return -1;
	 }
}

int GetTcUserNumber(int nlsock, int nlpid)
{
	char user_query[sizeof(TCUSERQUERY)] = {0};
	char recvbuf[MAX_MSG_LEN] = {0};
	int len = 0;

	BuildUserQueryMsg(user_query, NETLINK_TC_USER_TOTAL);

	Log(LOG_LEVEL_NORMAL, "+++++++++before SendMessage+++socket: %d++pid: %d+++", nlsock, nlpid);
	SendMessage(nlsock, user_query, sizeof(user_query), nlpid);

	Log(LOG_LEVEL_NORMAL, "+++++++++after SendMessage++++++++");
	len = RecvMessage(nlsock, recvbuf, MAX_MSG_LEN);
	if (len > 0)
	{
		//PNETLINK_MSG_HDR msghdr = (PNETLINK_MSG_HDR)recvbuf;
		PTCUSERRESP prsp = (PTCUSERRESP)recvbuf;
		total_user = prsp->total_user;

		return total_user;
	}
	else
	{
		Log(LOG_LEVEL_WARN, "recv message error");
		return -1;
	}
}

int GetTcUserData(int nlsock, int nlpid)
{
	char user_query[sizeof(TCUSERQUERY)] = {0};
	char recvbuf[MAX_MSG_LEN] = {0};
	int len = 0;
	int ret = 0;

	Log(LOG_LEVEL_NORMAL, "+++++++++GetTcUserData++++++++");
	BuildUserQueryMsg(user_query, NETLINK_TC_USER_INFO);
	SendMessage(nlsock, user_query, sizeof(user_query), nlpid);
	//Log(LOG_LEVEL_NORMAL, "+++++++++after SendMessage++++++++");
	while (1)
	{
		len = RecvMessage(nlsock, recvbuf, MAX_MSG_LEN);
		if (len > 0)
		{
			if ( (ret = ProcessNLMessage(recvbuf)) != 0)
			{
				continue;
			}
			else
			{
				break;
			}
		}
		else
		{
			Log(LOG_LEVEL_WARN, "recv message error");
			return -1;
		}
	}

	return 0;

}
#if 1
int GetTcUserInfo(int nlsock, int nlpid, char *result, int *count)
{ 
	char user_query[sizeof(TCUSERQUERY)] = {0};
	char recvbuf[MAX_MSG_LEN] = {0};
	int len = 0;
	int offset = 0;
	int ret = 0;
	int userinfo_len = sizeof(TCUSERINFO) - sizeof(NETLINK_MSG_DEF);

	// construct query message 
	BuildUserQueryMsg(user_query, NETLINK_TC_USER_TOTAL);

	Log(LOG_LEVEL_NORMAL, "^^^^^^^^^send query ");
	// send query
	SendMessage(nlsock, user_query, sizeof(user_query), nlpid);
	while (1)
	{
		// recv mesage
		len = RecvMessage(nlsock, recvbuf, sizeof(recvbuf));
		if (len > 0)
		{
			// send to client
			if ( (ret = ProcessNLMessage(recvbuf)) != 0)
			{
				continue;
			}
			else
			{
				break;
			}
		}
		else if (len == 0)
		{
			Log(LOG_LEVEL_NORMAL, "recv all user info success ^_^");
			break;
		}
		else
		{
			Log(LOG_LEVEL_ERROR, "RecvMessage() < 0 ");
			break;
		}
	} 
	
	return 0;
}
#endif

