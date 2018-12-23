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
#ifndef __KERNEL_NETLINK_H__
#define __KERNEL_NETLINK_H__

#include <linux/skbuff.h>
#define NETLINK_PCP 25
#define NETLINK_ICMP_ERROR 26
#define NETLINK_DHCP6 27

int get_icmp_error_pid(void);

/** 
* @fn int send_nlmessage(char *message, int msglen)
* @brief send netlink message
* 
* @param[in] nl_sk netlink socket
* @param[in] pid user id
* @param[in] message netlink message
* @param[in] msglen  lengeth of message
* @retval void
*/
void send_nlmsg(struct sock * nl_sk, int pid, char *message, int msglen);

/** 
 * @fn Netlink_kernel_create
 * @brief wrapp netlink_kernel_create()
 * 
 * @retval void
 */
void Netlink_kernel_create(void);

/** 
 * @fn Netlink_kernel_release
 * @brief wrapp netlink_kernel_release()
 * 
 * @retval void
 */
void Netlink_kernel_release(void);
#endif
