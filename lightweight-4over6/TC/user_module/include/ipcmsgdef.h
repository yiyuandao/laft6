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
#ifndef _IPCMSGDEF_H_
#define _IPCMSGDEF_H_

//
//在线用户数量查询消息请求
//
#define IPC_MSG_ONLINE_USER_NUM_REQ		0x01
//
//在线用户数量查询消息应答
//
#define IPC_MSG_ONLINE_USER_NUM_RSP		0x02
//
//在线用户IP映射关系查询消息请求
//
#define IPC_MSG_ONLINE_USER_MAP_REQ		0x03
//
//在线用户IP映射关系查询消息应答
//
#define IPC_MSG_ONLINE_USER_MAP_RSP		0x04
//
//在线用户添加请求
//
#define IPC_MSG_ONLINE_USER_ADD_REQ		0x05
//
//在线用户添加应答
//
#define IPC_MSG_ONLINE_USER_ADD_RSP		0x06


#define IPC_RESULT_SUCCESS 		0
#define IPC_RESULT_EXIST		1
#define IPC_RESULT_FAIL			2

//
//IPC消息头
//
typedef struct _tag_ipcmsghdr
{
	unsigned char ipcmsgtype;
}IPC_MSG_HDR, *PIPC_MSG_HDR;
//
//在线用户数量查询请求消息
//
typedef struct _tag_ipc_msg_user_num_req
{
	IPC_MSG_HDR ipcmsghdr;
}IPC_MSG_USER_NUM_REQ, *PIPC_MSG_USER_NUM_REQ;
//
//在线用户数量查询应答消息
//
typedef struct _tag_ipc_msg_user_num_rsp
{
	IPC_MSG_HDR 		ipcmsghdr;
	unsigned long 		usernum;
}IPC_MSG_USER_NUM_RSP, *PIPC_MSG_USER_NUM_RSP;

//
//在线用户IP映射关系查询请求消息
//
typedef struct _tag_ipc_msg_user_map_req
{
	IPC_MSG_HDR 		ipcmsghdr;
	unsigned long 		userpagenum;	//当前一页需要请求的最大用户数量
	unsigned long 		userpageoffset;		//当前请求位置的偏移量
}IPC_MSG_USER_MAP_REQ, *PIPC_MSG_USER_MAP_REQ;

//
//在线用户IP映射关系查询应答消息
//
typedef struct _tag_ipc_msg_user_map_rsp
{
	IPC_MSG_HDR 		ipcmsghdr;
	unsigned long		usernum;	//当前请求的实际用户量
	unsigned long 		usertablelen;//映射关系表的长度
	unsigned char 		usertable[1];//变长 
	//eg.
	//ipcmsgusermaprsplen = sizeof(IPC_MSG_USER_MAP_RSP) - sizeof(unsigned char) + usertablelen;
	//pipcmsgusermaprsp = (PIPC_MSG_USER_MAP_RSP)malloc(ipcmsgusermaprsplen);
}IPC_MSG_USER_MAP_RSP, *PIPC_MSG_USER_MAP_RSP;

//
//在线用户添加请求消息
//
typedef struct _tag_ipc_msg_user_add_req
{
	IPC_MSG_HDR 		ipcmsghdr;
	unsigned long		usernum;	//当前添加的实际用户量
	unsigned long 		usertablelen;//映射关系表的长度
	unsigned char 		usertable[1];//变长
}IPC_MSG_USER_MAP_ADD_REQ, *PIPC_MSG_USER_MAP_ADDR_REQ;


typedef struct _tag_user_map_info
{
	unsigned short sport;
	unsigned short eport;
	unsigned int extip;
	unsigned char useraddr[16];
	unsigned long lefttime;
}USER_MAP_INFO, *PUSER_MAP_INFO; 

//
//在线用户添加应答消息
//
typedef struct _tag_user_map_info_add_result
{
	USER_MAP_INFO	usermapinfo;
	int				result;		//添加的结果
}USER_MAP_INFO_ADD_RESULT, *PUSER_MAP_INFO_ADD_RESULT;

//
//在线用户添加应答消息
//
typedef struct _tag_ipc_msg_user_add_rsp
{
	IPC_MSG_HDR 		ipcmsghdr;
	unsigned long		usernum;	//当前添加失败的实际用户量
	unsigned long 		usertablelen;//映射关系表的长度
	unsigned char 		usertalbe[1];//变长USER_MAP_INFO_ADD_RESULT[]
}IPC_MSG_USER_MAP_ADD_RSP, *PIPC_MSG_USER_MAP_ADD_RSP;
#endif
