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
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "log.h"
#include "configure.h"
#include "dnsproxy.h"
#include "global.h"
#include "pcpportsetmsgdef.h"
#include "pcpportsetentity.h"
#include "sig.h"
#include "netlink_user.h"
#include "icmperror.h"
#include "commit.h"
#include "get_tc_addr.h"
#include "getglobalipv6address.h"

void Initialize(PCONFIG config, PLAFT_NAT_INFO natinfo)
{
	memset(config, 0, sizeof(CONFIG));
	memset(&natinfo, 0, sizeof(natinfo));
}


void DebugConfig(PCONFIG config)
{
    int i = 0;
    printf("loaclipv6:");
    for (i = 0; i < sizeof(config->localipv6); i ++)
    {
        printf("0x%02x, ", config->localipv6[i]);
    }
    printf("\n");
    printf("gwipv6:");
    for (i = 0; i < sizeof(config->gwipv6); i ++)
    {
        printf("0x%02x, ", config->gwipv6[i]);
    }
    printf("\n");
    printf("virgwipv6:");
    for (i = 0; i < sizeof(config->virgwipv6); i ++)
    {
        printf("0x%02x, ", config->virgwipv6[i]);
    }
    printf("\n");
    printf("dnssrvipv6:");
    for (i = 0; i < sizeof(config->dnssrvipv6); i ++)
    {
        printf("0x%02x, ", config->dnssrvipv6[i]);
    }
    printf("\n");
    printf("pcpport:%d\n", config->pcpport);
    printf("logflag %d\n", config->Logflag);
}

void DebugNatinfo(PLAFT_NAT_INFO natinfo)
{
    int i = 0;
    printf("Local IPv6 Addr is: ");
    for (i = 0; i < sizeof(natinfo->IPv6LocalAddr); i ++)
    {
        printf("0x%02x, ", natinfo->IPv6LocalAddr[i]);
    }
    printf("\n");
	
    printf("Remote IPv6 Addr is: ");
    for (i = 0; i < sizeof(natinfo->IPv6RemoteAddr); i ++)
    {
        printf("0x%02x, ", natinfo->IPv6RemoteAddr[i]);
    }
    printf("\n");
	
    printf("Public IPv4 Addr is :");
    for (i = 0; i < sizeof(natinfo->PubIP); i ++)
    {
        printf("0x%02x, ", natinfo->PubIP[i]);
    }
    printf("\n");
	
    printf("Low Port is: %d\n", natinfo->LowPort);
    printf("High Port is: %d\n", natinfo->HighPort);

}


void  ChildExitHandle(int sig)
{
	int status;
	printf("Child Exit Handle Start.............\n");
	while (wait3(&status, WNOHANG, NULL) > 0){
	
	}
}

void PCPWorkThread(void *param)
{
	PPCPPORTSET_CTX_T pcpctx = (PPCPPORTSET_CTX_T)param;
	int iRet = 0;
	LAFT_NAT_INFO natinfo; 
	CONFIG config;
	
	Initialize(&config, &natinfo);
	if(LoadConfig(&config, &natinfo) < 0){
		Log(LOG_LEVEL_ERROR, "Load Config File Error!!");
		return;
	}
	
	while (1)
	{
		iRet = PCPRequestHandle(pcpctx, pcpctx->uiLeftTime);
		if (iRet < 0)
			break;
		printf("CLIENT SEND RENEW WILL BE AFTER %d SECONDS\n", pcpctx->uiLeftTime / 2);

		if (natinfo.LowPort == pcpctx->usStartPort && 
			natinfo.HighPort == pcpctx->usEndPort &&
			memcmp(natinfo.PubIP, &pcpctx->uiExternIPv4, 4) == 0)
		{
			sleep(pcpctx->uiLeftTime / 2);
			continue;
		}
		natinfo.LowPort = pcpctx->usStartPort;
		natinfo.HighPort = pcpctx->usEndPort;
		memcpy(natinfo.PubIP, &pcpctx->uiExternIPv4, 4);
		memcpy(natinfo.IPv6LocalAddr, pcpctx->aucCltIPv6, 16);
		Log(LOG_LEVEL_NORMAL, "Send Message to Kernel Start......");
		SendMessage((char*)&natinfo, sizeof(natinfo));
		Log(LOG_LEVEL_NORMAL, "Send Message to Kernel End......");
		sleep(pcpctx->uiLeftTime / 2);
	}
	
}

void newPcpRequest(void *param)
{
	PPCPPORTSET_CTX_T pcpctx = (PPCPPORTSET_CTX_T)param;
	int iRet = 0;
	LAFT_NAT_INFO natinfo; 
	CONFIG config;
	
	Initialize(&config, &natinfo);
	if(LoadConfig(&config, &natinfo) < 0){
		Log(LOG_LEVEL_ERROR, "Load Config File Error!!");
		return;
	}
	
	iRet = PCPRequestHandle(pcpctx, pcpctx->uiLeftTime);
	if (iRet < 0)
	{
		return;
	}
	printf("CLIENT SEND RENEW WILL BE AFTER %d SECONDS\n", pcpctx->uiLeftTime / 2);

	natinfo.LowPort = pcpctx->usStartPort;
	natinfo.HighPort = pcpctx->usEndPort;
	memcpy(natinfo.PubIP, &pcpctx->uiExternIPv4, 4);
	memcpy(natinfo.IPv6LocalAddr, pcpctx->aucCltIPv6, 16);
	Log(LOG_LEVEL_NORMAL, "Send Message to Kernel Start......");
	SendMessage((char*)&natinfo, sizeof(natinfo));
	Log(LOG_LEVEL_NORMAL, "Send Message to Kernel End......");

}

void *handleIcmp6Error(void *param)
{
	int sk;
	socklen_t addrlen;
	int nrecv;
	unsigned char aucBuf[2048] = {0};
	struct sockaddr_in6 client;
	char addr6[128] = {0};
 	PPCPPORTSET_CTX_T pcpctx = (PPCPPORTSET_CTX_T)param;
	pthread_detach(pthread_self());

	sk = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if (sk == -1)
	{
		perror("socket");
		return NULL;
	}

	addrlen = sizeof(client);
	while (1)
	{
		nrecv = recvfrom(sk, aucBuf, sizeof(aucBuf), 0,
				(struct sockaddr *)(&client), &addrlen);
		if (nrecv < 8)
		{
			continue;
		}
		else if (nrecv < 0)
			break;

		// icmpv6 reserver filed 0xffffffff
		if (*(unsigned *)(aucBuf + 4) == 0xffffffff)
		{
			Log( LOG_LEVEL_WARN,
				"find icmp6 error %s",
				inet_ntop(AF_INET6, client.sin6_addr.s6_addr, addr6, sizeof(addr6)));
			PCPPortSetSrvAddr(pcpctx, client.sin6_addr.s6_addr);
			newPcpRequest(param);	
		}

	}
	
	return NULL;
}

void *handleIcmpv4Error(void *param)
{
	int icmp_pid = 0;
	int icmp_sock_fd = 0;
	int nrecv = 0;
#define MAX_MESSAGE_LEN 200
	unsigned char recvbuf[MAX_MESSAGE_LEN] = {0};
	struct iphdr *iph = NULL;
	int nSnd = 0;
	
	icmp_sock_fd = InitNetLinkSocket(NETLINK_ICMP_ERROR, &icmp_pid);
	SendNLMessage(icmp_sock_fd, icmp_pid, "init icmp socket", sizeof("init icmp socket"));

	while (1)
	{
		nrecv = RecvNLMessage(icmp_sock_fd, (char *)recvbuf, MAX_MESSAGE_LEN);
		if (nrecv > 0)
		{
			iph = (struct iphdr *)recvbuf;
			
			nSnd = IcmpErrorSend(0,
				htonl(iph->saddr),
				recvbuf, 
				nrecv, 
				1);
			Log(LOG_LEVEL_WARN, "IcmpErrorSend = %d", nSnd);
		}
		else
		{
			Log(LOG_LEVEL_WARN, "RecvNLMessage err"); 
		}
	}

}

int resolv_tc_domain(unsigned char *tc_address)
{
	int dhcp_pid = 0;
	int dhcp_sock_fd = 0;
	int nrecv = 0;
	int ret = 0;
	unsigned char recvbuf[MAX_MESSAGE_LEN] = {0};
	fd_set set;
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	dhcp_sock_fd = InitNetLinkSocket(NETLINK_DHCP6_OPTION, &dhcp_pid);
	SendNLMessage(dhcp_sock_fd, dhcp_pid, "init dhcp6 option socket", sizeof("init dhcp6 socket"));

	FD_ZERO(&set);
	FD_SET(dhcp_sock_fd , &set);

	ret = select(dhcp_sock_fd + 1, &set, NULL, NULL, &timeout);
	if (ret > 0)
	{
		nrecv = RecvNLMessage(dhcp_sock_fd, (char *)recvbuf, MAX_MESSAGE_LEN);
		if (nrecv > 0)
		{
			get_tc_addr((char *)recvbuf, tc_address);
			CloseNetLinkSocket(dhcp_sock_fd);
			return 0;
		}
		else
		{
			Log(LOG_LEVEL_WARN, "RecvNLMessage err"); 
			CloseNetLinkSocket(dhcp_sock_fd);
			return -1;
		}
	}
	else if (ret == 0)
	{
		// timeout
		Log(LOG_LEVEL_WARN, "RecvNLMessage timeout"); 
	}
	else
	{
		perror("select");
	}

	CloseNetLinkSocket(dhcp_sock_fd);
	return -1;
}

int main(int argc, char* argv[])
{
	CONFIG config;
	LAFT_NAT_INFO natinfo;
	PPCPPORTSET_CTX_T pcpctx = NULL;
	int result = 0;
	pthread_t pidpcpwork = 0;
	pthread_t piddns = 0;
	pthread_t thid_icmp6 = 0;
	pthread_t thid_icmp4 = 0;
	unsigned char tc_addr[16] = {0};
	unsigned char ti_addr[2][16];
	unsigned char *pserveraddr = NULL;
	unsigned char *plocalipv6addr = NULL;

	signal_init();	
	Initialize(&config, &natinfo);
	OpenLog(NULL, 0x01);

	if(LoadConfig(&config, &natinfo) < 0){
		Log(LOG_LEVEL_ERROR, "Load Config File Error!!");
		return -1;
	}
	
	DebugConfig(&config);
	DebugNatinfo(&natinfo);

#if 1
	// get local ipv6 address
	if ( (result = get_global_ipv6addr(ti_addr, 2)) > 0)
	{
		Log(LOG_LEVEL_NORMAL, "get local global ipv6 address");
		print_buf(ti_addr[0], 16);
		plocalipv6addr = ti_addr[0];
	}
	else
	{
		plocalipv6addr = config.localipv6;
	}
	// parse tc domain name
	if (resolv_tc_domain(tc_addr) == 0)
	{
		pserveraddr = tc_addr;
	}
	else
	{
		pserveraddr = config.gwipv6;
	}
#endif
	pserveraddr = config.gwipv6;
	pcpctx = PCPPortSetContextInit(plocalipv6addr,
		//config.gwipv6,
		pserveraddr,
		config.pcpport);
	if (pcpctx == NULL)
	{
		Log( LOG_LEVEL_ERROR,
			"PCPPortSetContextInit Error!!");
		return -1;
	}

	result = pthread_create(&pidpcpwork, NULL, (void *)PCPWorkThread, (void *)pcpctx);
	if(result != 0){
		Log(LOG_LEVEL_ERROR, "Pthread PCPWorkThread Create Error!!");
		return -1;
	}

	// process icmp6 error
	result = pthread_create(&thid_icmp6, NULL, handleIcmp6Error, (void *)pcpctx);
	if(result != 0){
		Log(LOG_LEVEL_ERROR, "Pthread handleIcmp6Error Create Error!!");
		return -1;
	}
#if 1
	// process icmpv4 error(packet too big)
	result = pthread_create(&thid_icmp4, NULL, handleIcmpv4Error, NULL);
	if(result != 0){
		Log(LOG_LEVEL_ERROR, "Pthread handleIcmpv4Error Create Error!!");
		return -1;
	}
#endif
	signal(SIGCHLD, ChildExitHandle);

	//Log(LOG_LEVEL_NORMAL, "Start Timer Thread......");
	gNatConfig.PcpPort = config.pcpport;
	memcpy(gNatConfig.SrcIpv6, config.localipv6, sizeof(config.localipv6));
	memcpy(gNatConfig.kpaliveipv6, config.gwipv6, sizeof(config.gwipv6));

	Log(LOG_LEVEL_NORMAL, "Dns Proxy Start......");
	memcpy(gNatConfig.DnsSrvIpv6, config.dnssrvipv6, sizeof(config.dnssrvipv6));
	result = pthread_create(&piddns, NULL, (void *)dns_start6, NULL);
	if(result != 0){
		Log(LOG_LEVEL_ERROR, "Pthread Dns Create Error!!");
		return -1;
	}
	Log(LOG_LEVEL_NORMAL, "Dns Proxy End......");

	//pthread_join(pidpcpwork, NULL);
	//pthread_join(piddns, NULL);
	while (1)
	{
		sleep(1);
		if (signal_judge())
			break;
	}
	
	//lefttime value is zero,delete the node
	PCPRequestHandle(pcpctx, 0);
	//destory pcp context
	PCPPortSetContextDestory(pcpctx);
	
	return 0;
}


