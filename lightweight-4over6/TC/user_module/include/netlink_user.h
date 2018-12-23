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
#ifndef __NETLINK_USER_H__
#define __NETLINK_USER_H__

#define NETLINK_TEST 25
/** 
 * @fn  InitNetLinkSocket
 * @brief create socket(sock_fd) and bind
 * 
 * @param[in] protocol netlink protocol
 * @param[out] nl_pid netlink pid for each thread
 * @retval 0 success
 * @retval -1 failure
 */
int InitNetLinkSocket(int protocol, int *nl_pid);

/** 
 * @fn  SendMessage
 * @brief send netlink msg
 * 
 * @param[in] sock_fd netlink socket
 * @param[int] nl_pid netlink pid for each thread
 * @param[in] message netlink message
 * @param[in] len  message length
 * @retval 0 success
 * @retval -1 failure
 */
int SendMessage(int sock_fd, char *message, int len, int nl_pid);

/** 
 * @fn  RecvMessage
 * @brief receive netlink message and handle message
 * 
 * @param[in] sock_fd netlink socket
 * @param[int] nl_pid netlink pid for each thread
 * @param[out] uiPublicIP external ipv4 address
 * @param[out] usStartPort start port
 * @retval 0 success
 * @retval -1 failure
 */
int RecvMessage(int sock_fd, char *MsgContent, unsigned int msglen);

/** 
 * @fn  CloseNetLinkSocket
 * @brief close netlink socket
 * 
 * @param[in] sock_fd netlink socket
 * @retval void
 */
void CloseNetLinkSocket(int sock_fd);
#endif
