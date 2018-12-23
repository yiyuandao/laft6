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
#ifndef _NLMESSAGE_H_
#define _NLMESSAGE_H_

#include "netlinkmsgdef.h"

#define MSG_ERR_SUCCESS				1
#define MSG_ERR_INTER				0
#define MSG_ERR_CANNOT_ALLOC_PORT 	-1
#define MSG_ERR_RECYCLED_PORT		-2
#define MSG_ERR_RESULT				-3
#define MSG_ERR_UNKNOWN				-4

typedef struct _tag_msgdebug
{
	NETLINK_MSG_DEF 		msgdef;
	char *msgcontent;
}MSGDEBUG, *PMSGDEBUG;

typedef struct _tag_netaddrinfo
{
	unsigned int 	extip;
	unsigned short  startport;
	unsigned short  endport;
}NETADDRINFO, *PNETADDRINFO;


/** 
 * @fn ConstructNetlinkMsg
 * @brief construct normal netlink NETLINK_INT message:
 * (NETLINK_DETECT_MSG, NETLINK_DELETE_NODE) 
 * 
 * @param[in] msg_type message type 
 * @param[in] cliIpv6Addr client ipv6 address
 * @param[out] buf netlink message
 * @param[in] lefttime live time
 * @retval void
 */
void ConstructNetlinkMsg(char *buf, int msg_type, unsigned char *cliIpv6Addr, unsigned long lefttime);

/** 
 * @fn ConstructAllocMsg
 * @brief construct netlink NETLINK_TUNNEL_MSG message(NETLINK_ALLOC_ADDR)
 * 
 * @param[in] cliIpv6Addr client ipv6 address
 * @param[in] type message type 
 * @param[in] ip external ipv4 address
 * @param[in] StartPort start port
 * @param[in] EndPort  end port
 * @param[out] buf netlink message
 * @retval void
 */   
void ConstructAllocMsg(char *buf, 
		unsigned char *cliIpv6Addr,
		unsigned long ip,
		unsigned short StartPort, 
		unsigned short EndPort, 
		unsigned long 	lefttime);

void ConstructTcStateQuery(char *buf);

void ConstructUninitMsg(PNETLINK_MSG_TCCONFIG pmsgconfig);

/** 
* @fn HandlMessage
* @brief handle netlink message
* 
* @param[in] msg netlink message
* @param[out] uiPublicIP external ipv4 address
* @param[out] usStartPort startport
* @retval 0 success
* @retval -1 failure
*/
int HandlMessage(int sock_fd, char *msg, char *MsgContent);

#endif
