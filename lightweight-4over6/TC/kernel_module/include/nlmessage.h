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
#include "tccorestat.h"

/** 
 * @fn ConstructIntMsg
 * @brief construct normal netlink NETLINK_INT message:
 * (NETLINK_RESULT, NETLINK_ALLOC_ADDR, NETLINK_UPDATE_MSG) 
 * 
 * @param[in] cliIpv6Addr client ipv6 address
 * @param[in] type message type 
 * @param[in] result  0: default 1: fail
 * @param[in] lefttime live time
 * @param[out] buf netlink message
 * @retval void
 */
void ConstructIntMsg(char *buf,
unsigned char *cliIpv6Addr,
int type,
unsigned char result,
unsigned long lefttime);
/** 
 * @fn ConstructTunMsg
 * @brief construct netlink NETLINK_TUNNEL_MSG message(NETLINK_DELETE_NODE)
 * 
 * @param[in] cliIpv6Addr client ipv6 address
 * @param[in] type message type 
 * @param[in] ip external ipv4 address
 * @param[in] start_port start port
 * @param[in] end_port  end port
 * @param[out] buf netlink message
 * @retval void
 */
void ConstructTunMsg(char *buf, unsigned char *cliIpv6Addr, int type, unsigned int ip, 
					unsigned short star_port, unsigned short end_port);


unsigned char* ConstructCrashPktMsg(struct sk_buff *skb, unsigned int *len);

unsigned char *ConstructTcStateMsg(PTCCORESTATINFO_T tccorestatinfo, unsigned int *len);

unsigned char *ConstructIcmpErrMsg(struct sk_buff *skb, unsigned int *len);

#endif
