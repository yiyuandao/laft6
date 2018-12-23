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
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "log.h"
#include "config.h"
#include "global.h"
#include "netlinkmsgdef.h"
#include "netlink_async.h"
#include "netlink_user.h"
#include "nlmessage.h"
#include "pcpthread.h"
#include "nlmessage.h"
#include "tcmgrinter.h"
#include "tcuserinfo.h"
#include "ipcsrv.h"
#include "ipcmsg.h"

time_t programStartTime = 0;

/** 
 * @fn   InitTc
 * @brief Init TC config
 * 
 */
void InitTc(void)
{
	NETLINK_MSG_TCCONFIG tcconfig;
	memset(&tcconfig, 0, sizeof(tcconfig));
	tcconfig.msghdr.msgdef = NETLINK_INIT_CONFIG;
	tcconfig.uibeginIP = GlobalCtx.Config.AddrPool.uiStartIp;
	Log(LOG_LEVEL_NORMAL, "begin:0x%x", tcconfig.uibeginIP);
	tcconfig.uiEndIP = GlobalCtx.Config.AddrPool.uiEndIp;
	Log(LOG_LEVEL_NORMAL, "End:0x%x", tcconfig.uiEndIP);
	tcconfig.uiOtherBeginIP = GlobalCtx.Config.BakupAddrPool.uiStartIp;
	Log(LOG_LEVEL_NORMAL, "Other begin:0x%x", tcconfig.uiOtherBeginIP);
	tcconfig.uiOtherEndIP = GlobalCtx.Config.BakupAddrPool.uiEndIp;
	Log(LOG_LEVEL_NORMAL, "Other end:0x%x", tcconfig.uiOtherEndIP);
	tcconfig.uiTunState = 1;
	tcconfig.usmtu = GlobalCtx.Config.tc_config.usmtu;
	Log(LOG_LEVEL_NORMAL, "MTU:%d", tcconfig.usmtu);
	memcpy(tcconfig.tc_addr, GlobalCtx.Config.tc_config.ucLocalIPv6Addr, 16);
	tcconfig.uiversion = GlobalCtx.Config.tc_config.uiversion;
	Log(LOG_LEVEL_NORMAL, "Version:%d", tcconfig.uiversion);
	tcconfig.usPortRange = ~GlobalCtx.Config.AddrPool.usPortMask + 1;
	tcconfig.uiMaxLifeTime = SUCCESS_LIFETIME;
	GlobalCtx.Config.usPortMask = GlobalCtx.Config.AddrPool.usPortMask;
	Log(LOG_LEVEL_NORMAL, 
		"PortRange:%d, PortMask:0x%x",
		tcconfig.usPortRange,
		GlobalCtx.Config.AddrPool.usPortMask);
	SendAsyncMessage((char *)&tcconfig, sizeof(tcconfig));
}


void UnInitTc(void)
{
	NETLINK_MSG_TCCONFIG tcconfig;
	ConstructUninitMsg(&tcconfig);

	SendAsyncMessage((char *)&tcconfig, sizeof(tcconfig));
}

/** 
 * @fn   sig_int
 * @brief handle signal SIGINT
 * 
 * @param[in] sig signal
 * 
 */
void sig_int(int sig)
{
	Log(LOG_LEVEL_NORMAL, "catch ctrl+c");
	ipc_server_stop();
	UnInitTc();
	exit(0);
}

/** 
 * @fn   sig_term
 * @brief handle signal SIGTERM
 * 
 * @param[in] sig signal
 * 
 */
void sig_term(int sig)
{
	Log(LOG_LEVEL_NORMAL, "catch SIGTERM");
	UnInitTc();
	exit(0);
}

/** 
 * @fn   sig_segv
 * @brief handle signal SIGSEGV
 * 
 * @param[in] sig signal
 * 
 */
void sig_segv(int sig)
{
	Log(LOG_LEVEL_NORMAL, "catch SIGSEGV");
	UnInitTc();
	exit(0);
}
#if 1
int TCTest(void)
{
	char *beginipv6 = "240c:f:0:ffff::0";

	char send_buf[200] = {0};
	char recv_buf[2000] = {0};
	int iRet = 0;
	NETADDRINFO netaddrinfo;
	unsigned int addrlen = sizeof(NETADDRINFO);
	int nl_pid = 0;
	int sock_fd;
	NETLINK_MSG_TUNCONFIG tcconfig;
	unsigned char ucbeginipv6[16] = {0};
	int *s = 0;
	int i = 0;
	Log(LOG_LEVEL_ERROR, "TEST START!");
	inet_pton(AF_INET6, beginipv6, ucbeginipv6);

#if 0
		sock_fd = InitNetLinkSocket(NETLINK_TEST, &nl_pid);
		if (sock_fd < 0)
		{
			Log(LOG_LEVEL_ERROR, "can not create netlink socket");
			return MSG_ERR_INTER;
			
		}
#endif
	for (i = 0; i < 923; i ++)
	{
		s = (int *)&ucbeginipv6[12];
		*s = htonl((*s));
		(*s) ++;
		*s = htonl((*s));

#if 1
		sock_fd = InitNetLinkSocket(NETLINK_TEST, &nl_pid);
		if (sock_fd < 0)
		{
			Log(LOG_LEVEL_ERROR, "can not create netlink socket");
			return MSG_ERR_INTER;
			
		}
#endif
		//construct detect msg
		ConstructNetlinkMsg(send_buf,
				NETLINK_DETECT_MSG,
				ucbeginipv6,
				SUCCESS_LIFETIME);

		//send detect msg
		if (SendMessage(sock_fd, send_buf, sizeof(send_buf), nl_pid) == 0)
		{ 
			//recvmsg and process message
			iRet = RecvMessage(sock_fd, recv_buf, sizeof(recv_buf));
			if (iRet < 0)
			{
				CloseNetLinkSocket(sock_fd);
				return MSG_ERR_INTER;
			}
			
			iRet = HandlMessage(sock_fd, recv_buf, (char *)&tcconfig);
			if (iRet != MSG_ERR_SUCCESS)
			{
				CloseNetLinkSocket(sock_fd);
				return iRet;
			}
			//send alloc_addr msg
			Log(LOG_LEVEL_ERROR, "send request allocate add");
			// construct msg
			ConstructAllocMsg(send_buf,
			tcconfig.user_addr,
			tcconfig.public_ip,
			tcconfig.start_port,
			tcconfig.end_port,
			//tcconfig.lefttime);
			1200);

			//send allocate address and port request
			iRet = SendMessage(sock_fd, send_buf, sizeof(send_buf), nl_pid);
			if (iRet < 0)
			{
				CloseNetLinkSocket(sock_fd);
				return MSG_ERR_INTER;
			}
			//return result
			iRet = RecvMessage(sock_fd, recv_buf, sizeof(recv_buf));
			if (iRet < 0)
			{
				CloseNetLinkSocket(sock_fd);
				return MSG_ERR_INTER;
			}
			iRet = HandlMessage(sock_fd, recv_buf, NULL);
			if (iRet != MSG_ERR_SUCCESS)
			{
				CloseNetLinkSocket(sock_fd);
				return iRet;
		}
		Log(LOG_LEVEL_ERROR, "IP:%x, startport:%d", tcconfig.public_ip, tcconfig.start_port);

		CloseNetLinkSocket(sock_fd);
		//return iRet;
	} 
	else
	{ 
		CloseNetLinkSocket(sock_fd);
		return MSG_ERR_INTER;
	} 

	}
	Log(LOG_LEVEL_ERROR, "TEST END!");

}
#endif
void *create_sigsegv(void *param)
{
	int *p = NULL;

	sleep(10);
	*p = 1;
}

void test_user_info(void *param)
{
	int sock_fd;
	int nl_pid;
	int count = 0;
	char recvbuf[1500];
	struct timeval start;
	struct timeval end;
	int nuser = 0;

	if ( (sock_fd = InitNetLinkSocket(22, &nl_pid)) < 0)
	{
		Log(LOG_LEVEL_ERROR, "can not create netlink socket");
		return MSG_ERR_INTER;
	}

	Log(LOG_LEVEL_NORMAL, "*******************get user info start........");
	//while (1)
	{
		//sleep(20);
		gettimeofday(&start, NULL);
		//GetTcUserInfo(sock_fd, nl_pid, recvbuf, &count);
		nuser = GetTcUserNumber(sock_fd, nl_pid);	
		gettimeofday(&end, NULL);


		Log(LOG_LEVEL_NORMAL, "********++++++++++++++user:%d++***********time: %lds..%ldus.....", nuser,
				end.tv_sec - start.tv_sec,
				end.tv_usec - start.tv_usec);
	}
}

int main(int argc, char *argv[])
{
	pthread_t pcpThread;
	pthread_t asyncThread;
	pthread_t test;
	
	signal(SIGINT, sig_int);
	signal(SIGTERM, sig_term);
	//signal(SIGSEGV, sig_segv);

	OpenLog(NULL, OUTPUT_CONSOLE);
	if (InitInterSocket() < 0)
	{
		Log(LOG_LEVEL_ERROR, "InitInterSocket ERROR");
		return -1;
	}
	Log(LOG_LEVEL_NORMAL, "entry start");
	// start up time
	programStartTime = time(NULL); 
	memset(&GlobalCtx, 0, sizeof(GLOBAL_CTX));
	//Load config file 
	if (LoadConfig(&GlobalCtx.Config) < 0)
	{
		Log(LOG_LEVEL_ERROR, "Load config file error!");
		return -1;
	}
	Log(LOG_LEVEL_NORMAL, "entry load config");
	//Init Address pool
	InitAddrPool(&GlobalCtx.AddrPoolList,
				 &GlobalCtx.Config.AddrPool);

	// init async netlink socket
	InitAsyncNetLinkSocket();
	
	// init tc
	InitTc();
		
	// start process pcp message
	if(pthread_create(&asyncThread, NULL, (void*)process_del_node, NULL) != 0)
	{
		Log(LOG_LEVEL_ERROR, "create async message thread failed");
		return -1;
	}

	// start process pcp message
	if(pthread_create(&pcpThread, NULL, (void*)pcp_start, NULL) != 0)
	{
		Log(LOG_LEVEL_ERROR, "create pcp thread failed");
		return -1;
	}
#if 0
	TCTest();		
#endif
	ipc_server_init(AF_INET, (void *)ipc_msg_handle);
	ipc_server_start();
	 //test get user info
	//test_user_info(NULL);
	// start process pcp message
#if 0
	if(pthread_create(&test, NULL, (void *)test_user_info, NULL) != 0)
	{
		Log(LOG_LEVEL_ERROR, "create test user info thread failed");
		return -1;
	}
#endif
	pthread_join(asyncThread, NULL);
	pthread_join(pcpThread, NULL);
#if 0
	pthread_join(test, NULL);
#endif 
	Log(LOG_LEVEL_NORMAL, "entry end");

	UnInitInterSocket();
	return 0;
}

