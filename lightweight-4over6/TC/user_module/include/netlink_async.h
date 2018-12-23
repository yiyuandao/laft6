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
#ifndef __NETLINK_ASYNC_H__
#define __NETLINK_ASYNC_H__

/** 
 * @fn  InitAsyncNetLinkSocket
 * @brief create socket(sock_fd) and bind
 * 
 * @param[in] None
 * @retval 0 success
 * @retval -1 failure
 */
int InitAsyncNetLinkSocket(void);

/** 
 * @fn  SendAsyncMessage
 * @brief send netlink msg
 * 
 * @param[in] message netlink message
 * @param[in] len  message length
 * @retval 0 success
 * @retval -1 failure
 */
int SendAsyncMessage(char *message, int len);

/** 
 * @fn  RecvAsyncMessage
 * @brief receive netlink message and handle message
 * 
 * @param[out] uiPublicIP external ipv4 address
 * @param[out] usStartPort start port
 * @retval 0 success
 * @retval -1 failure
 */
int RecvAsyncMessage(void);

/** 
 * @fn  CloseAsyncNetLinkSocket
 * @brief close netlink socket
 * 
 * @retval void
 */
void CloseAsyncNetLinkSocket(void);

/** 
 * @fn  process_del_node
 * @brief handle delete node message
 * 
 * @retval void
 */
void process_del_node(void *param);
#endif
