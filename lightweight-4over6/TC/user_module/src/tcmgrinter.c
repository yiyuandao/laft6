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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <asm/types.h>
#include <linux/socket.h>
#include <errno.h>
#include "tcmgrinter.h"
#include "log.h"
#include "netlink_user.h"

pthread_t interthreadId;
int sock = 0;

int InitInterSocket(void)
{

	int ret = 0;
	struct sockaddr_in6 server_addr;
	
	int sockaddr_len = 0;
	
	sock = socket(AF_INET6,SOCK_DGRAM,0);
	if(sock <= 0)
	{
		Log(LOG_LEVEL_ERROR, "socket error");
		return -1;
	}
	server_addr.sin6_family = AF_INET6;
	server_addr.sin6_port = htons(60005);
	server_addr.sin6_addr = in6addr_any;
	sockaddr_len = sizeof(server_addr);
	if (bind(sock, (struct sockaddr *)&server_addr, sockaddr_len) != 0)
	{
		Log(LOG_LEVEL_ERROR, "bind error");
		return -1;

	}
	ret = pthread_create(&interthreadId, NULL, (void *)ThreadInterfaceProc, NULL);
	if (ret < 0)
	{
		Log(LOG_LEVEL_ERROR, "pthread_create error");
		return -1;
	}
	return 1;
}

int UnInitInterSocket(void)
{
	close(sock);
	pthread_cancel(interthreadId);
}

void PrintTcState(PTCCORESTATINFO_T tccoreinfo)
{
	Log(LOG_LEVEL_NORMAL, "User:%lld", tccoreinfo->llStat[USER_SIZE]);
	Log(LOG_LEVEL_NORMAL, "TX:%lldBytes", tccoreinfo->llStat[TX_BYTE]);
	Log(LOG_LEVEL_NORMAL, "RX:%lldBytes", tccoreinfo->llStat[RX_BYTE]);
	Log(LOG_LEVEL_NORMAL, "TX:%lldPackets", tccoreinfo->llStat[TX_PACKET]);
	Log(LOG_LEVEL_NORMAL, "RX:%lldPackets", tccoreinfo->llStat[RX_PACKET]);	
}




void StatesReply(PINTER_MSG pstatereply)
{
	char buf[20] = {0};
	int iret = 0;
	TCCORESTATINFO_T tcinfo;
	int nl_pid = 0;
	int sock_fd;

	sock_fd = InitNetLinkSocket(NETLINK_TEST, &nl_pid);
	if (sock_fd < 0)
	{
		Log(LOG_LEVEL_ERROR, "can not create netlink socket");
		return;
		
	}
	
	int len = sizeof(tcinfo);
	pstatereply->interhdr.flaghdr = HDR_FLAG;
	pstatereply->interhdr.cmdtype = CMD_TYPE_STATE_REPLY;
	ConstructTcStateQuery(buf);
	iret = SendMessage(sock_fd, buf, sizeof(buf), nl_pid);
	{
		RecvMessage(sock_fd, &tcinfo, &len);
		PrintTcState(&tcinfo);
		sprintf(pstatereply->msg,
			"USER:%04lld\nRX:%08lldBytes\nTX:%08lldBytes\nRX:%08lldPackets\nTX:%08lldPackets", 
			tcinfo.llStat[USER_SIZE],
			tcinfo.llStat[RX_BYTE],
			tcinfo.llStat[TX_BYTE],
			tcinfo.llStat[RX_PACKET],
			tcinfo.llStat[TX_PACKET]);
		
	}
	CloseNetLinkSocket(sock_fd);
}

void ThreadInterfaceProc(void *param)
{
	struct sockaddr_in6 addrSrv;
	int len = 0, result = 0, sendlen = 0;
	unsigned char recvBuf[256] = {0};
	PINTER_HDR  pinterhdr = NULL;
	INTER_MSG statereply;
	
	memset(&addrSrv, 0, sizeof(struct sockaddr_in6));

	len=sizeof(addrSrv);  
	
	while (1)
	{
		result = recvfrom(sock, (char *)recvBuf, sizeof(recvBuf), 0, (struct sockaddr*)&addrSrv, &len);
		if (result < 0)
		{
			Log( LOG_LEVEL_ERROR, "recvfrom error over");
			break;
		}
		
		if (result < sizeof(INTER_HDR))
		{
			continue;
		}
		Log(LOG_LEVEL_NORMAL, "receive query request!");
		pinterhdr = (PINTER_HDR)recvBuf;
		if (pinterhdr->flaghdr == HDR_FLAG)
		{
			switch (pinterhdr->cmdtype)
			{
			case CMD_TYPE_STATE_REQ:
				{
					memset(&statereply, 0, sizeof(INTER_MSG));
					StatesReply(&statereply);
					sendlen = sendto(sock, &statereply, sizeof(INTER_MSG), 0, (struct sockaddr * )&addrSrv, len);
					if (sendlen < 0)
					{
						Log(LOG_LEVEL_ERROR, "sendto error");
					}
				}
				break;
			default:
				break;
			}
		}
	}
}

