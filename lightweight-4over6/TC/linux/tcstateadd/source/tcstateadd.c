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

typedef struct _tag_user_map_link
{
	struct _tag_user_map_link 	*pnext;
	USER_MAP_INFO 	 			usermapinfo;
}USER_MAP_NODE, *PUSER_MAP_NODE;

static int line_parase(char *line, PUSER_MAP_INFO pusermapinfo)
{
	char *s = NULL;
	int offset = 0;

	if (line == NULL)
		return -1;
	if (line[0] == '#')
		return 0;
	s = strtok(line, "	");
	if (s == NULL)
		return -1;
	//client ipv6
	inet_pton(AF_INET6, s, pusermapinfo->useraddr);
	offset += (strlen(s) + 1);	
	s = strtok(&line[offset], "	");
	if (s == NULL)
		return -1;
	//extern ipv4
	inet_pton(AF_INET, s, &pusermapinfo->extip);
	offset += (strlen(s) + 1);	
	s = strtok(&line[offset], "	");
	if (s == NULL)
		return -1;
	offset += (strlen(s) + 1);	
	//port range
	s = strtok(s, "-");
	if (s == NULL)
		return -1;
	pusermapinfo->sport = htons(atoi(s));
	s = s + strlen(s) + 1;
	if (s == NULL)
		return -1;
	pusermapinfo->eport = htons(atoi(s));
	//lefttime
	s = strtok(&line[offset], "	");
	if (s == NULL)
		return -1;
	pusermapinfo->lefttime = htonl(atoi(s));

	return 1;
}


static int ip_map_file_read(PUSER_MAP_NODE pUserMapList, int *ListSize)
{	
	FILE *pf = NULL;
	char *end = NULL;
	USER_MAP_INFO usrinfo;
	char line[1024] = {0};
	int ret = 0;
	PUSER_MAP_NODE pNode = NULL, pCur = NULL;
	int count = 0;
	
	pf = fopen("ipmap", "r+");
	if (pf == NULL) {
		return -1;
	}
	while (1) {
		end = fgets(line, sizeof(line), pf);
		if (end == NULL)
			break;
		ret = line_parase(line, &usrinfo);
		if (ret == 0)
		{
			continue;
		}
		
		if (ret < 0)
		{
			fclose(pf);
			return -1;
		}
		
		if (ret > 0)
		{
			pCur = pUserMapList;
			pNode = (PUSER_MAP_NODE)malloc(sizeof(USER_MAP_NODE));
			if (pNode == NULL){
				fclose(pf);
				return -1;
			}
			memcpy(&pNode->usermapinfo, &usrinfo, sizeof(usrinfo));
			pNode->pnext = NULL;
			if (pUserMapList->pnext == NULL){
				pUserMapList->pnext = pNode;	
			}else {
				pCur->pnext = pNode;
			}
			count ++;
			pCur = pNode;
		}
	}
	fclose(pf);
	
	*ListSize = count;
	return 1;
}

static int ipc_user_map_add_req_create(PIPC_MSG_USER_MAP_ADDR_REQ *pmsg, 
										PUSER_MAP_INFO pUserMapTable, 
										unsigned int tablesize)
{
	int msglen = 0;
	msglen = sizeof(IPC_MSG_USER_MAP_ADD_REQ) - sizeof(unsigned char) + tablesize * sizeof(USER_MAP_INFO);
	*pmsg = (PIPC_MSG_USER_MAP_ADDR_REQ)malloc(msglen);
	(*pmsg)->ipcmsghdr.ipcmsgtype = IPC_MSG_ONLINE_USER_ADD_REQ;
	(*pmsg)->usernum = htonl(tablesize);
	(*pmsg)->usertablelen = htonl(tablesize * sizeof(USER_MAP_INFO));
	memcpy(&(*pmsg)->usertalbe[0], pUserMapTable, tablesize * sizeof(USER_MAP_INFO));

	return msglen;
}

void  print_tc_user_map_result(PIPC_MSG_USER_MAP_ADD_RSP pipcmapaddrsp)
{
	int usernum = 0;
	int indx = 0;
	PUSER_MAP_INFO_ADD_RESULT pmapinfo = NULL;
	char ipv6addr[128] = {0};
	char ipv4addr[64] = {0};
	static int no = 0;
	char *result[] = {"success","exsit","fail"};
	printf("Index\tUser IPv6\tExtern IPv4\tPort Range\tLefttime\tResult\n");
	usernum = htonl(pipcmapaddrsp->usernum);

	while (indx < htonl(pipcmapaddrsp->usertablelen))
	{
		pmapinfo = (PUSER_MAP_INFO_ADD_RESULT)&pipcmapaddrsp->usertalbe[indx];
		inet_ntop(AF_INET6, pmapinfo->usermapinfo.useraddr, ipv6addr, sizeof(ipv6addr));
		inet_ntop(AF_INET, &pmapinfo->usermapinfo.extip, ipv4addr, sizeof(ipv4addr));
		printf("%d\t%s\t%s\t%d-%d\t%d\t%s\n", 
			no++, 
			ipv6addr,
			ipv4addr,
			htons(pmapinfo->usermapinfo.sport),
			htons(pmapinfo->usermapinfo.eport),
			htonl(pmapinfo->usermapinfo.lefttime),
			result[htonl(pmapinfo->result)]);		
		indx += sizeof(USER_MAP_INFO_ADD_RESULT);
	}
}

int main(int argc, char* argv[])
{	
	int iret = 0;
	USER_MAP_NODE UserMapList;
	PUSER_MAP_NODE pNode = NULL, pNext = NULL;
	int listsize = 0;
	PUSER_MAP_INFO pmaplist = NULL;
	int off = 0;
	PIPC_MSG_USER_MAP_ADDR_REQ pAddMapMsgReq = NULL;
	int msglen = 0;
	char recvbuf[1500] = {0};
	if (argv != 2)
	{
		printf("first edit configuration file: ipmap\n");
		printf("useage: %s 127.0.0.1\n", argv[0]);
		return 0;
	}
	iret = ip_map_file_read(&UserMapList, &listsize);
	if (iret < 0)
	{
		printf("Not found ipmap file!\n");
		return 0;
	}
	pmaplist = (PUSER_MAP_INFO)malloc(listsize * sizeof(USER_MAP_INFO));
	pNode = UserMapList.pnext;
	
	for (off = 0; off < listsize; off ++)
	{	
		pNext = pNode->pnext;
		memcpy(&pmaplist[off], &pNode->usermapinfo, sizeof(USER_MAP_INFO));
		free(pNode);
		pNode = NULL;
		pNode = pNext;
	}
	
	msglen = ipc_user_map_add_req_create(&pAddMapMsgReq, pmaplist, listsize);
	struct sockaddr_in srvaddr;
	memset(&srvaddr, 0, sizeof(srvaddr));
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = htons(55501);
	srvaddr.sin_addr.s_addr = inet_addr(argv[1]);
	iret = ipc_clt_send((unsigned char *)pAddMapMsgReq,
		msglen, 
		recvbuf, 
		sizeof(recvbuf), 
		(struct sockaddr*)&srvaddr, 1);
	if (iret < 0){
		return -1;
	}
	print_tc_user_map_result((PIPC_MSG_USER_MAP_ADD_RSP)recvbuf);	
	free(pmaplist);
	getc(stdin);
}
