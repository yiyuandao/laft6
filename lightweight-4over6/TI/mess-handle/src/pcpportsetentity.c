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
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
//#include <sys/time.h>
#include <time.h>
#include <asm/ioctls.h> 
#include <asm/byteorder.h> 
#include <unistd.h>
#include "pcpportsetmsgdef.h"
#include "pcpportsetentity.h"

#if 0
//0x1301a8c0->0xc0a80113
static unsigned int ipConvert(unsigned int ip)
{
	unsigned int a = ip & 0xff000000;
	unsigned int b = ip & 0x00ff0000;
	unsigned int c = ip & 0x0000ff00;
	unsigned int d = ip & 0x000000ff;

	return (d << 24) | (c << 8) | (b >> 8) | (a >> 24);
}
#endif

static void MESSAGE_DUMP(unsigned char *pBuf, int bufLen)
{
	int ibytes = 0;    

	printf("PCP MESSAGE_DUMP:\n");

	if ((NULL == pBuf) || (0 == bufLen))
	{
		return;
	}

	for (ibytes = 0; ibytes < bufLen; ibytes++)
	{
		printf("%02X ", pBuf[ibytes]);
		if ((ibytes + 1) % 8 == 0 )
		{
			printf("\n");
		}
	}
	printf("\n");

	return;
}


static void ErrorPrint(unsigned char ucErrCode)
{
	char *pAcErrMsg[ERR_MAX] = {
		"ERR_SUCCESS",
		"ERR_UNSUPP_VERSION",
		"ERR_NOT_AUTHORIZED",
		"ERR_MALFORMED_REQUEST",
		"ERR_UNSUPP_OPCODE",
		"ERR_UNSUPP_OPTION",
		"ERR_MALFORMED_OPTION",
		"ERR_NETWORK_FAILURE",
		"ERR_NO_RESOURCES",
		"ERR_UNSUPP_PROTOCOL",
		"ERR_USER_EX_QUOTA",
		"ERR_CANNOT_PROVIDE_EXTERNAL",
		"ERR_ADDRESS_MISMATCH",
		"ERR_EXCESSIVE_REMOTE_PEERS"
	};

	if (ucErrCode < ERR_MAX)
		printf("!!!Errcode:0x%x,%s\n", ucErrCode, pAcErrMsg[ucErrCode]);
}


//pcp context initialization
PPCPPORTSET_CTX_T PCPPortSetContextInit(unsigned char *pucCltIPv6,
										unsigned char *pucSrvIPv6,
										unsigned short usPCPPort)					
{
	PPCPPORTSET_CTX_T pPcpCtx_t = NULL;
	int bErr = 0;
	int iRet = 0;
	struct timeval timeoutval = {0, 0};
	//unsigned long mode = 1;
	
	//allocate pcp client context
	pPcpCtx_t = (PPCPPORTSET_CTX_T)malloc(sizeof(PCPPORTSET_CTX_T));
	if (pPcpCtx_t == NULL)
	{
		bErr = 1;
		goto ERR;
	}
	//zero
	memset(pPcpCtx_t, 0, sizeof(PCPPORTSET_CTX_T));
	
	memcpy(pPcpCtx_t->aucCltIPv6, pucCltIPv6, 16);
	memcpy(pPcpCtx_t->aucSrvIPv6, pucSrvIPv6, 16);
	pPcpCtx_t->usPcpPort = usPCPPort;
	//initialize left time
	pPcpCtx_t->uiLeftTime = 24 * 60 * 60;//24h
	//initialize socket	
	pPcpCtx_t->PcpSocket = socket(AF_INET6, SOCK_DGRAM, 0);
	if (pPcpCtx_t->PcpSocket < 0)
	{
		bErr = 1;
		goto ERR;
	}
	//set receive timeout value
	timeoutval.tv_sec = RCV_TIMEOUT;
	iRet = setsockopt(pPcpCtx_t->PcpSocket,
		SOL_SOCKET,
		SO_RCVTIMEO,
		(char*)&timeoutval,
		sizeof(timeoutval)); 
	if (iRet < 0)
	{
		bErr = 1;
		goto ERR;
	}
	//set send timeout value
	timeoutval.tv_sec = SND_TIMEOUT;
	iRet = setsockopt(pPcpCtx_t->PcpSocket,
		SOL_SOCKET,
		SO_SNDTIMEO, 
		(char*)&timeoutval,
		sizeof(timeoutval)); 
	if (iRet < 0)
	{
		bErr = 1;
		goto ERR;
	}

ERR:
	//error flow
	if (bErr)
	{
		if (pPcpCtx_t->PcpSocket > 0)
		{
			close(pPcpCtx_t->PcpSocket);
		}

		if (pPcpCtx_t)
		{
			free(pPcpCtx_t);
			pPcpCtx_t = NULL;
		}
	}

	return pPcpCtx_t;
}	

//pcp message response
int PCPResponseHandle(PPCPPORTSET_CTX_T pPcpCtx_t,
							unsigned char *pucResBuf,
							unsigned int uiLen,
							unsigned char* pucErrCode)
{
	PPCP_RSP_T pPcpRsp_t = NULL;
	unsigned int uiExtIP = 0;
	int bInvaild = 0;
	unsigned int uiSrvCurEpoch = 0, uiCltEpochDelt = 0, uiSrvEpochDelt = 0;
	
	MESSAGE_DUMP(pucResBuf, uiLen);
	//check length
	if ((uiLen < 3) || (uiLen > 1024) || (uiLen % 4) != 0)
	{
		printf("THE PACKET LENGTH IS ERROR!\n");
		return RET_DORP;
	}
	pPcpRsp_t = (PPCP_RSP_T)pucResBuf;
	//check R bit
	if ((pPcpRsp_t->ucR | PCP_RESP) == 0)
	{
		printf("THE PACKET R BIT IS ERROR!\n");
		return RET_DORP;
	}
	//check opcode
	if (pPcpRsp_t->ucOpCode != OP_CODE_MAP_PORT_SET)
	{
		printf("THE PACKET OPCODE IS ERROR!\n");
		return RET_DORP;
	}	
	pPcpCtx_t->uiLeftTime = htonl(pPcpRsp_t->uiReqLiftTime);
	printf("CLIENT LEFTTIME:%d SECONDS\n", pPcpCtx_t->uiLeftTime);

	//initialize current epoch time value
	if (pPcpCtx_t->uiCltCurEpochTime == 0)
	{
		pPcpCtx_t->uiCltCurEpochTime = pPcpCtx_t->uiCltPreEpochTIme = time(NULL);
	}
	else
	{
		//record current epoch time
		pPcpCtx_t->uiCltCurEpochTime = time(NULL);
	}	

	uiSrvCurEpoch = htonl(pPcpRsp_t->uiEpochTime);
	//initialize server last epoch time
	if (pPcpCtx_t->uiSrvLastEpochTime == 0)
	{
		pPcpCtx_t->uiSrvLastEpochTime = uiSrvCurEpoch;
		printf("SERVER LASTEPOCHTIME:%d SECONDS\n", pPcpCtx_t->uiSrvLastEpochTime);
	}
	else
	{
		// If the current PCP server Epoch time value
		//(current_server_time) is less than the previously received PCP
		//	server Epoch time value (previous_server_time) then the client
		//	treats the Epoch time value as obviously invalid (time should
		//	not go backwards)
		if (pPcpCtx_t->uiSrvLastEpochTime > uiSrvCurEpoch)
		{
			//invaild(time should not go backwards)
			bInvaild = 1;
			printf("THE PCP SERVER IS UNUSUAL, THE TIME SHOULD NOT GO BACKWARDS!\n");
		}
		else
		{	
			uiCltEpochDelt = pPcpCtx_t->uiCltCurEpochTime - pPcpCtx_t->uiCltPreEpochTIme;
			uiSrvEpochDelt = uiSrvCurEpoch - pPcpCtx_t->uiSrvLastEpochTime;
			//If client_delta+2 < server_delta - server_delta/16
			//or server_delta+2 < client_delta - client_delta/16
			//	then the client treats the Epoch time value as invalid,
			//else the client treats the Epoch time value as valid
			if ((uiCltEpochDelt + 2 < uiSrvEpochDelt - uiSrvEpochDelt / 16) || 
				(uiSrvEpochDelt + 2 < uiCltEpochDelt - uiCltEpochDelt / 16))
			{
				//invaild
				bInvaild = 1;
				printf("THE PCP SERVER IS UNUSUAL, THE EPOCH TIME VALUE IS INVAILD!\n");
			}
		}
		//update epoch
		//previous_client_time = current_client_time
		pPcpCtx_t->uiCltPreEpochTIme = pPcpCtx_t->uiCltCurEpochTime;
		//previous_server_time = current_server_time
		pPcpCtx_t->uiSrvLastEpochTime = uiSrvCurEpoch;

		if (bInvaild)
			return RET_INVAILD_EPOCH;
	}
	//error flow
	if (pPcpRsp_t->ucResultCode != ERR_SUCCESS)
	{
		*pucErrCode = pPcpRsp_t->ucResultCode;
		ErrorPrint(*pucErrCode);

		return RET_MSG_ERR;
	}
	else
	{
		unsigned short usPortRange = 0;
		pPcpCtx_t->usMask = htons(pPcpRsp_t->option_mps_t.usPortRangeMask);
		pPcpCtx_t->usValue = htons(pPcpRsp_t->option_mps_t.usPortRangeValue);
		memcpy(&uiExtIP, &pPcpRsp_t->mps_rsp_t.aucAEIPAddr[12], sizeof(int));

		usPortRange = ~pPcpCtx_t->usMask + 1;
		printf("PORT LENGTH :%d\n", usPortRange);
#if defined (__BIG_ENDIAN_BITFIELD)
		pPcpCtx_t->uiExternIPv4 = uiExtIP;
		printf("big PORT MASK:0X%X\n", pPcpCtx_t->usMask);
		printf("big PORT VALUE:0X%X\n", pPcpCtx_t->usValue);
		printf("big EXT IP:0X%X\n", pPcpCtx_t->uiExternIPv4 );

		pPcpCtx_t->usStartPort = pPcpCtx_t->usValue;
		printf("big PORT START :%d\n", pPcpCtx_t->usStartPort);
		pPcpCtx_t->usEndPort = pPcpCtx_t->usValue + usPortRange - 1;
		printf("big PORT END :%d\n", pPcpCtx_t->usEndPort);
#elif defined(__LITTLE_ENDIAN_BITFIELD)

		//pPcpCtx_t->uiExternIPv4 = htonl(uiExtIP);
		pPcpCtx_t->uiExternIPv4 = uiExtIP;
		printf("litt PORT MASK:0X%X\n", pPcpCtx_t->usMask);
		printf("litt PORT VALUE:0X%X\n", pPcpCtx_t->usValue);
		printf("litt EXT IP:0X%X\n", pPcpCtx_t->uiExternIPv4 );

		pPcpCtx_t->usStartPort = pPcpCtx_t->usValue;
		printf("little PORT START :%d\n", pPcpCtx_t->usStartPort);
		pPcpCtx_t->usEndPort = pPcpCtx_t->usValue + usPortRange - 1;
		printf("little PORT END :%d\n", pPcpCtx_t->usEndPort);
#endif
	}

	return RET_SUCCESS;
}

//pcp message request
int PCPRequestHandle(PPCPPORTSET_CTX_T pPcpCtx_t,
				unsigned int uiLeftTime)
{
	int result;
	struct sockaddr_in6 AddrSrv;
	unsigned int uiExIP = 0;
	unsigned int ustemp = 0xffff;
	unsigned char ucErrCode;
	char sendBuf[MAX_PCPPORTSET_MSG_LEN] = {0};
	char recvBuf[MAX_PCPPORTSET_MSG_LEN] = {0};
	char acIPv6[64] = {0};
	PPCP_REQ_T pPcpReq_t = NULL;
	socklen_t len = 0;

	memset(&AddrSrv, 0, sizeof(struct sockaddr_in6));
	AddrSrv.sin6_family = AF_INET6;
	memcpy(AddrSrv.sin6_addr.s6_addr, pPcpCtx_t->aucSrvIPv6, 16);
	AddrSrv.sin6_port = htons(PCP_PORT);
	len = sizeof(AddrSrv);     

	pPcpReq_t = (PPCP_REQ_T)sendBuf;
	pPcpReq_t->ucVer = PCP_VER;
	pPcpReq_t->ucR = PCP_REQ;
	pPcpReq_t->ucOpCode = OP_CODE_MAP_PORT_SET;
	pPcpReq_t->uiReserved = 0;

	pPcpReq_t->uiReqLiftTime = htonl(uiLeftTime);

	memcpy(pPcpReq_t->aucCLTIPv6Addr, pPcpCtx_t->aucCltIPv6, 16);

	memset(pPcpReq_t->mps_req_t.aucSEIPAddr, 0, 16);
	pPcpReq_t->option_mps_t.ucOptionCode = 0;
	pPcpReq_t->option_mps_t.ucReserved = 0;
	pPcpReq_t->option_mps_t.usOptionLen = htons(4);
	pPcpReq_t->option_mps_t.usPortRangeValue = htons(pPcpCtx_t->usValue);
	pPcpReq_t->option_mps_t.usPortRangeMask = htons(pPcpCtx_t->usMask);
	uiExIP = htonl(pPcpCtx_t->uiExternIPv4);

	// construct ipv4 mapped ipv6 address ::ffff:0.0.0.0
	memcpy(&pPcpReq_t->mps_req_t.aucSEIPAddr[sizeof(pPcpReq_t->aucCLTIPv6Addr) - sizeof(int) - 2], &ustemp, 2);
	memcpy(&pPcpReq_t->mps_req_t.aucSEIPAddr[sizeof(pPcpReq_t->aucCLTIPv6Addr) - sizeof(int)], 
		&uiExIP, 
		sizeof(int));
	//initialize retransmit time value
	int t = INIT_RETRANSITME;

	while (1)
	{
		result = sendto(pPcpCtx_t->PcpSocket,
			sendBuf,
			sizeof(PCP_REQ_T),
			0,
			(struct sockaddr *)&AddrSrv,
			len);

		if (result < 0)
		{
			printf("SEND ERROR %d\n", errno);
			return -1;
		}

		inet_ntop(AF_INET6, AddrSrv.sin6_addr.s6_addr, acIPv6, sizeof(acIPv6));
		printf("PCP RESQUEST-->>%s, %d\n", acIPv6, result);
		MESSAGE_DUMP((unsigned char *)sendBuf, result);

		//receive response
		result = recvfrom(pPcpCtx_t->PcpSocket,
			recvBuf,
			sizeof(recvBuf),
			0,
			(struct sockaddr *)&AddrSrv,
			&len);
		if (result <= 0)
		{
			if (errno == EAGAIN)
			{	
				//compute retransmit time value
				t = 2 * t;
				if (t > MAX_RETRANSTIME)
				{
					t = MAX_RETRANSTIME;
				}
				printf("RECEIVE PACKET IS TIMEOUT, THE NEXT RETRANSMIT TIME VALUE IS %d SECONDS!\n", t);
				//wait next retransmit timeout value
				sleep(t);

				continue;
			}
			printf("RECVFROM ERROR %d\n", errno);
			return -1;
		}	
		else
		{
			int iRet = 0;
			
			//match the server address 
			if (memcmp(AddrSrv.sin6_addr.s6_addr, pPcpCtx_t->aucSrvIPv6, 16) != 0)
			{
				printf("ADDRESS IS MISMATCH!\n");
				return -1;
			}
			//reset retransmit time value
			t = INIT_RETRANSITME;
			inet_ntop(AF_INET6, AddrSrv.sin6_addr.s6_addr, acIPv6, sizeof(acIPv6));
			printf("PCP RESPONSE<<--%s, %d\n", acIPv6, result);
			iRet = PCPResponseHandle(pPcpCtx_t, 
				(unsigned char *)recvBuf, 
				result,
				&ucErrCode);
			if (iRet < 0)
			{
				if (iRet == RET_INVAILD_EPOCH)
				{
					//the epoch time value is invalid, send renew message immediately
					continue;
				}
				else if (iRet == RET_MSG_ERR)
				{
					return 1;
				}

				return -1;
			}
			break;
		}
	}

	return 1;
}


//pcp context destory
void PCPPortSetContextDestory(PPCPPORTSET_CTX_T pPCPCtx_t)
{
	if (pPCPCtx_t)
	{
		if (pPCPCtx_t->PcpSocket > 0)
		{
			close(pPCPCtx_t->PcpSocket);
		}

		if (pPCPCtx_t)
		{
			free(pPCPCtx_t);
			pPCPCtx_t = NULL;
		}
	}
}

void PCPPortSetSrvAddr(PPCPPORTSET_CTX_T pPCPCtx_t, unsigned char *SrvAddress)
{
	memcpy(pPCPCtx_t->aucSrvIPv6, SrvAddress, 16);
}
