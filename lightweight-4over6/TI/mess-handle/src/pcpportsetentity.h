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


#ifndef _PCPPORTSETENTITY_H_
#define _PCPPORTSETENTITY_H_

#define	SND_TIMEOUT 1
#define RCV_TIMEOUT 3

#define RET_SUCCESS			0
#define RET_DORP			-1
#define RET_INVAILD_EPOCH	-2
#define RET_MSG_ERR			-3


//PCP client context
typedef struct _tag_pcpportset_ctx_t
{
	unsigned int	uiExternIPv4; //extern ipv4 address
	unsigned short	usStartPort; //start of port
	unsigned short	usEndPort; //end of port 
	unsigned short  usValue; //port value
	unsigned short  usMask;	//port mask

	unsigned int	uiLeftTime; //left time value
	unsigned int	uiSrvLastEpochTime;	//server epoch time value
	unsigned int	uiCltCurEpochTime;	//client current epoch time value
	unsigned int	uiCltPreEpochTIme;  //client previous epoch time value

	unsigned char	aucCltIPv6[16]; //client ipv6 address
	unsigned char	aucSrvIPv6[16]; //pcp server ipv6 address
	unsigned short	usPcpPort;	//pcp server port 

	int			PcpSocket;	//socket
}PCPPORTSET_CTX_T, *PPCPPORTSET_CTX_T;

PPCPPORTSET_CTX_T PCPPortSetContextInit(unsigned char *pucCltIPv6,
										unsigned char *pucSrvIPv6,
										unsigned short usPCPPort);

int PCPResponseHandle(PPCPPORTSET_CTX_T pPcpCtx_t,
					  unsigned char *pucResBuf,
					  unsigned int uiLen,
					  unsigned char* pucErrCode);

int PCPRequestHandle(PPCPPORTSET_CTX_T pPcpCtx_t,
					 unsigned int uiLeftTime);

void PCPPortSetContextDestory(PPCPPORTSET_CTX_T pPCPCtx_t);

void PCPPortSetSrvAddr(PPCPPORTSET_CTX_T pPCPCtx_t, unsigned char *SrvAddress);

#endif
