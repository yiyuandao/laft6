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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>

#include "netlinkmsgdef.h"
#include "nlmessage.h"
#include "netlink_user.h"
#include "newpcpmsgdef.h"
#include "log.h"
#include "global.h"
#include "optionlist.h"

#define SUCCESS 					0
#define PCP_VERSION 				1
#define PCP_REQUEST 				0
#define PCP_RESPONSE 				1
#define DELETE_NODE_TIME 			0

#define ERROR_LIFETIME 				1800 
#define MIN_REQUEST_LEN 			2
#define MAX_REQUEST_LEN 			1024

extern time_t programStartTime;

/** 
 * @fn updateTunnelTree
 * @brief update tunnel status
 * 
 * @param[in] pPcpRequest pcp request message
 * @param[out] uiPublicIP external ipv4 address
 * @param[out] usStartPort startport
 * @retval 0 success
 * @retval -1 failure
 */
int updateTunnelTree(PPCP_REQ_T pPcpRequest, unsigned int *uipublicIP, unsigned short *usStartPort)
{
	char send_buf[200] = {0};
	char recv_buf[512] = {0};
	char log[200] = {0};
	char client_v6[60] = {0};
	char ip[16] = {0};
	int iRet = 0;
	NETADDRINFO netaddrinfo;
	unsigned int addrlen = sizeof(NETADDRINFO);
	NETLINK_MSG_TUNCONFIG tcconfig;
	int nl_pid = 0;
	int sock_fd;

	sock_fd = InitNetLinkSocket(NETLINK_TEST, &nl_pid);
	if (sock_fd < 0)
	{
		Log(LOG_LEVEL_ERROR, "can not create netlink socket");
		return MSG_ERR_INTER;
		
	}
	//construct detect msg
	ConstructNetlinkMsg(send_buf,
			NETLINK_DETECT_MSG,
			pPcpRequest->aucCLTIPv6Addr,
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
		Log(LOG_LEVEL_NORMAL, "send request allocate add");
		// construct msg
		ConstructAllocMsg(send_buf,
		tcconfig.user_addr,
		tcconfig.public_ip,
		tcconfig.start_port,
		tcconfig.end_port,
		tcconfig.lefttime);

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
		
		*uipublicIP = tcconfig.public_ip;
		*usStartPort = tcconfig.start_port;
		
		// syslog user info
		 {
			 int ipv4 = htonl(tcconfig.public_ip);
			 sprintf(log, "- LAFT6:UserbasedA [- - %s %s - %d %d]",
					 inet_ntop(AF_INET6, tcconfig.user_addr, client_v6, sizeof(client_v6)),
					 inet_ntop(AF_INET, &ipv4, ip, sizeof(ip)),
					 //tcconfig.public_ip,
					 tcconfig.start_port,
					 tcconfig.end_port);
			 syslog(LOG_INFO | LOG_LOCAL3, "%s\n", log);
		 }

		CloseNetLinkSocket(sock_fd);
		return iRet;
	} 
	else
	{ 
		CloseNetLinkSocket(sock_fd);
		return MSG_ERR_INTER;
	} 
}

static void print_buf(char *buf, int len)
{
	int i;

	for (i = 0; i < len -1; i++)
	{
		printf("%02x ", buf[i]);
	}
	printf("%02x \n", buf[i]);
}

/** 
 * @fn delTunnel
 * @brief delete tunnel 
 * 
 * @param[in] pPcpRequest pcp request message
 * @param[out] uiPublicIP external ipv4 address
 * @param[out] usStartPort startport
 * @retval 0 success
 * @retval -1 failure
 */
int delTunnel(PPCP_REQ_T pPcpRequest, unsigned int *uipublicIP, unsigned short *usStartPort)
{
	char send_buf[200] = {0};
	char recv_buf[512] = {0};
	char log[200] = {0};
	char client_v6[60] = {0};
	char ip[16] = {0};
	NETADDRINFO tunconfig;
	int nl_pid = 0;
	int sock_fd;
	int iRet = 0;
	
	sock_fd = InitNetLinkSocket(NETLINK_TEST, &nl_pid);
	if (sock_fd < 0)
	{
		Log(LOG_LEVEL_ERROR, "can not create netlink socket");
		return MSG_ERR_INTER;
		
	}
	//construct delete node msg
	ConstructNetlinkMsg(send_buf,
			NETLINK_DELETE_NODE,
			pPcpRequest->aucCLTIPv6Addr,
			htonl(pPcpRequest->uiReqLiftTime));
	
	if (SendMessage(sock_fd, send_buf, sizeof(send_buf), nl_pid) == 0)
	{ 
		//recvmsg and process message
		iRet = RecvMessage(sock_fd, (char *)recv_buf, sizeof(recv_buf));
		if (iRet < 0)
		{
			CloseNetLinkSocket(sock_fd);
			return MSG_ERR_INTER;
		}
		iRet = HandlMessage(sock_fd, recv_buf, &tunconfig);
		if (iRet != MSG_ERR_SUCCESS)
		{
			CloseNetLinkSocket(sock_fd);
			return MSG_ERR_INTER;
		}

		*uipublicIP = tunconfig.extip;
		*usStartPort = tunconfig.startport;

#if 1
		// syslog user info
		 {
			 int ipv4 = htonl(tunconfig.extip);
			 sprintf(log, "- LAFT6:UserbasedW [- - %s %s - %d %d]",
					 inet_ntop(AF_INET6, pPcpRequest->aucCLTIPv6Addr, client_v6, sizeof(client_v6)),
					 inet_ntop(AF_INET, &ipv4, ip, sizeof(ip)),
					 //tunconfig.public_ip,
					 tunconfig.startport,
					 tunconfig.endport);
			 syslog(LOG_INFO | LOG_LOCAL3, "%s\n", log);
		 }
#endif

		CloseNetLinkSocket(sock_fd);
		return MSG_ERR_SUCCESS;
	} 
	else
	{ 
		CloseNetLinkSocket(sock_fd);
		return -1;
	}
}

int check_opcode_map_port_set(PPCP_REQ_T pPcpRequest, int ireqLen, struct sockaddr_in6 *pcliAddr, POPTIONLIST list)
{
	// Port_Range option
	if (option_list_search_option(list, pPcpRequest->option_mps_t.ucOptionCode) < 0)
	{
		Log(LOG_LEVEL_ERROR, "OPtion protocol error");
		return ERR_UNSUPP_OPTION;
	}

	// address mismatch
	if (memcmp(pPcpRequest->aucCLTIPv6Addr, pcliAddr->sin6_addr.s6_addr, 16))
	{
		Log(LOG_LEVEL_ERROR, "address mismatch error");
		return ERR_ADDRESS_MISMATCH;
	}

	// option code valid and option lenght invalid
	// packet length != sizoef(*pPcpRequest)
	if ( ((pPcpRequest->option_mps_t.ucOptionCode == 0) &&
		ntohs(pPcpRequest->option_mps_t.usOptionLen) != 4) ||
			sizeof(*pPcpRequest) != ireqLen)
	{
		Log(LOG_LEVEL_ERROR, "malformed request");
		return 	ERR_MALFORMED_REQUEST;
	}

	return ERR_SUCCESS;

}

/** 
 * @fn checkPcpRequest
 * @brief sanity check pcp request
 * 
 * @param[in] pPcpRequest pcp request message
 * @param[in] ireqLen the length of pcp request
 * @param[in] pcliAddr client ipv6 address
 * @retval ERR_SUCCESS success
 * @retval other failure
 */
int checkPcpRequest(PPCP_REQ_T pPcpRequest, int ireqLen, struct sockaddr_in6 *pcliAddr)
{
	int ret = 0;
	// check pcp version
	if (pPcpRequest->ucVer != PCP_VERSION)
	{
		Log(LOG_LEVEL_ERROR, "version error");
		return 	ERR_UNSUPP_VERSION;
	}

	POPTIONLIST plist = option_list_search_opcode(phead, pPcpRequest->ucOpCode);
	if (plist == NULL)
	{
		Log(LOG_LEVEL_ERROR, "OPcode error");
		return ERR_UNSUPP_OPCODE;
	}

	switch (pPcpRequest->ucOpCode)
	{
		case OP_CODE_MAP_PORT_SET:
			{
				ret = check_opcode_map_port_set(pPcpRequest, ireqLen, pcliAddr, plist);
			}
			break;
		default:
			Log(LOG_LEVEL_ERROR, "unexpect happend");
			break;
	}
	return ret;
}


static int handlPCPRespone(int isendSockFd, unsigned char *msg,
		int ireqLen, 
		unsigned char Result,
		unsigned int uiExternIP,
		unsigned short usPort, 
		struct sockaddr_in6 *pcliAddr)
{
	PPCP_RSP_T pPcpRsp = (PPCP_RSP_T)msg;
	int iTmp = 0;

	pPcpRsp->ucR = PCP_RESPONSE;
	pPcpRsp->ucResultCode = Result;
	// 12 bytes reserved
	memset(pPcpRsp->aucReserved, 0, 12);

	if (Result != SUCCESS)
	{
		Log(LOG_LEVEL_NORMAL, "error msg!!!!!!!!!!!!!!!!!!!!!!!");
		pPcpRsp->uiReqLiftTime = htonl(ERROR_LIFETIME);
		//epoch time
		pPcpRsp->uiEpochTime = htonl(time(NULL) - programStartTime);
		// reset
		pPcpRsp->option_mps_t.usPortRangeValue = 0;
		pPcpRsp->option_mps_t.usPortRangeMask = 0;

		//pad pcprequest, align on 4 bytes
		if (ireqLen < MAX_REQUEST_LEN)
		{
			// equal ireqLen %4
			iTmp = ireqLen & 3;

			if (iTmp != 0)
			{
				ireqLen += 4 - iTmp;
			}
		}
		else if (ireqLen > MAX_REQUEST_LEN)
		{
			ireqLen = MAX_REQUEST_LEN;
		}
	}
	else
	{
		unsigned short temp = 0xffff;
		pPcpRsp->uiReqLiftTime = htonl(SUCCESS_LIFETIME);
		pPcpRsp->uiEpochTime = htonl(time(NULL) - programStartTime);

		uiExternIP = htonl(uiExternIP);
		memcpy(pPcpRsp->mps_rsp_t.aucAEIPAddr + 10, (char *)&temp, 2);
		memcpy(pPcpRsp->mps_rsp_t.aucAEIPAddr + 12, (char *)&uiExternIP, 4);
		pPcpRsp->option_mps_t.usPortRangeValue = htons(usPort);
		Log(LOG_LEVEL_NORMAL, "PortRangeValue:%d", usPort);

		// default port mask == 0xf800
		pPcpRsp->option_mps_t.usPortRangeMask = htons(GlobalCtx.Config.AddrPool.usPortMask);
		Log(LOG_LEVEL_NORMAL, "PortRangeMask:0x%x", GlobalCtx.Config.AddrPool.usPortMask);

	}

	if (sendto(isendSockFd, pPcpRsp, ireqLen, 0, (struct sockaddr *)pcliAddr, sizeof(*pcliAddr)) < 0)
	{
		Log(LOG_LEVEL_ERROR, "func:%s line:%d sendto() error", __FUNCTION__, __LINE__);
		return -1;
	}

	return 1;
}

int handle_opcode_map_port_set(PPCP_REQ_T pPcpRequest, 
								unsigned char *pcpmsg,
								int socket, 
								int ireqLen,
								int result, 
								struct sockaddr_in6 *pcliAddr,
								unsigned int *puiPublicIP, 
								unsigned short *pusStartPort)
{
	int iRet = 0;

	// update lifetime
	if (pPcpRequest->uiReqLiftTime != DELETE_NODE_TIME)
	{
		iRet = updateTunnelTree(pPcpRequest, puiPublicIP, pusStartPort);
		if (iRet <= 0)
		{	
			Log(LOG_LEVEL_ERROR, "line : %d update tunnel failed %d", __LINE__, iRet);

			if (iRet == MSG_ERR_CANNOT_ALLOC_PORT)
			{
				result = ERR_CANNOT_PROVIDE_EXTERNAL;
			}
		}
		Log(LOG_LEVEL_NORMAL, "line : %d ip: 0x%x: start port: %u", __LINE__, *puiPublicIP, *pusStartPort);
	} // end if (pPcpRequest->uiReqLiftTime != DELETE_NODE_TIME)
	else
	{
		if (delTunnel(pPcpRequest, puiPublicIP, pusStartPort) < 0) 
		{
			Log(LOG_LEVEL_ERROR, "line : %d del tunnel failed", __LINE__);
			*puiPublicIP = 0;
			*pusStartPort = 0;
			//return;

		}
	}

	handlPCPRespone(socket, pcpmsg, ireqLen, 
			result, *puiPublicIP, *pusStartPort, 
			pcliAddr);
}

/** 
 * @fn handlePcpRequest
 * @brief handl pcp request 
 * 
 * @param[in] pcpmsg pcp request message
 * @param[in] ireqLen the length of pcp request
 * @param[in] isendSockFd socket for sending data
 * @param[in] pcliAddr client ipv6 address
 * @retval void
 */
void handlePcpRequest(unsigned char *pcpmsg, int ireqLen, int isendSockFd, struct sockaddr_in6 *pcliAddr)
{
	int iRet = 0;
	int result = 0;
	unsigned int uiPublicIP = 0;
	unsigned short usStartPort = 0;
	PPCP_REQ_T pPcpRequest = (PPCP_REQ_T)pcpmsg;

	// check R bit and request len
	if (pPcpRequest->ucR == PCP_RESPONSE || ireqLen < MIN_REQUEST_LEN)
	{
		Log(LOG_LEVEL_ERROR, "file: %s func: %s line: %d drop the packet R bit == PCP_REPONSE or len(packet) < 2", 
				__FILE__, __FUNCTION__, __LINE__);
		return;
	}

	// sanity check
	result = checkPcpRequest(pPcpRequest, ireqLen, pcliAddr);
	if (result == ERR_SUCCESS)
	{
		switch (pPcpRequest->ucOpCode)
		{
			case OP_CODE_MAP_PORT_SET:
				{
					handle_opcode_map_port_set(pPcpRequest, pcpmsg, 
									isendSockFd, ireqLen, 
									result, pcliAddr, 
									&uiPublicIP, &usStartPort);		
					return;
				}
				break;
			default:
				break;
		}


	} // end if (iRet == ERR_SUCCESS)
	else
	{
		handlPCPRespone(isendSockFd, pcpmsg, ireqLen, 
				result, uiPublicIP, usStartPort, 
				pcliAddr);
	}
}
