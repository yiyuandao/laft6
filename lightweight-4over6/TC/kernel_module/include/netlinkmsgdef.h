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
#ifndef _NETLINKMSGDEF_H_
#define _NETLINKMSGDEF_H_

typedef enum netlink_msg_def
{
	NETLINK_RESULT = 0,
	NETLINK_DELETE_NODE = 1, //delete msg type
	NETLINK_ALLOC_ADDR,		 //allocate address & port msg type
	NETLINK_DETECT_MSG,		 //detect msg type
	NETLINK_UPDATE_MSG,		 //update timer msg type
	NETLINK_INIT_CONFIG,      // init tc
	NETLINK_CRASH_MSG,
	NETLINK_TC_STATE,
	NETLINK_UNINIT_CONFIG,
	NETLINK_ICMP_ERR_MSG,
	NETLINK_TC_USER_INFO, // query user info
	NETLINK_TC_USER_TOTAL, // total user msg
	NETLINK_MAX
}NETLINK_MSG_DEF;

typedef struct _tag_netlinkmsghdr
{
	NETLINK_MSG_DEF 		msgdef;
}NETLINK_MSG_HDR, *PNETLINK_MSG_HDR;

typedef struct _tag_netlinkmsginit
{
	NETLINK_MSG_HDR 		msghdr;
	unsigned char			ucResult;////1,exist;0,noexist;2,other
	unsigned char 			cli_addr[16];
	unsigned long 			lefttime;
}NETLINK_MSG_INIT, *PNETLINK_MSG_INIT;

typedef struct _tag_netlinkmsgtcconfig
{
	NETLINK_MSG_HDR 		msghdr;
	unsigned int	uiversion;
	unsigned short	usmtu;
	unsigned char 	tc_addr[16];
	unsigned int 	uibeginIP;
	unsigned int 	uiEndIP;
	unsigned int 	uiTunState;
	unsigned short 	usPortRange;
	unsigned int 	uiOtherBeginIP;
	unsigned int 	uiOtherEndIP;
	unsigned int 	uiMaxLifeTime;
}NETLINK_MSG_TCCONFIG, *PNETLINK_MSG_TCCONFIG;

typedef struct _tag_netlinkmsgtunconfig
{
	NETLINK_MSG_HDR 		msghdr;
	unsigned char 			user_addr[16];
	unsigned int 			public_ip;
	unsigned short 			start_port;
	unsigned short 			end_port;
	unsigned long 			lefttime;
}NETLINK_MSG_TUNCONFIG, *PNETLINK_MSG_TUNCONFIG;

typedef struct _tag_netlinkmsgcrashpacket
{
	NETLINK_MSG_HDR 		msghdr;
	unsigned int 			uiLen;
	unsigned char			pkt[1];
}NETLINK_MSG_CRASHPKT, *PNETLINK_MSG_CRASHPKT;

typedef struct _tag_netlinkmsgicmperr
{
	NETLINK_MSG_HDR 		msghdr;
	unsigned int 			uiLen;
	unsigned char			pkt[1];
}NETLINK_MSG_ICMPERR, *PNETLINK_MSG_ICMPERR;

typedef struct _tag_tcstate_query
{
	NETLINK_MSG_HDR 		msghdr;
}TCSTATEQUERY, *PTCSTATEQUERY;

typedef struct _tag_tcstate_resp
{
	NETLINK_MSG_HDR 		msghdr;
	unsigned int 			msglen;
	unsigned char 			content[1];
}TCSTATERSP, *PTCSTATERSP;

typedef struct _tag_userinfo
{
	unsigned short 			start_port;
	unsigned short 			end_port;
	unsigned int 			public_ip;
	unsigned char 			user_addr[16];
	unsigned long 			lefttime;
}USERINFO, *PUSERINFO;

typedef struct _tag_tcuser_query
{
	NETLINK_MSG_HDR 		msghdr;
}TCUSERQUERY, *PTCUSERQUERY;

typedef struct _tag_tcuserinfo
{
	NETLINK_MSG_HDR 		msghdr;
	unsigned int 			total_user;
	unsigned int 			msglen;
	USERINFO	user[50];
}TCUSERINFO, *PTCUSERINFO;

#endif
