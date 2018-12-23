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
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/netlink.h>

#include "commit.h"
#include "log.h"

#define NETLINK_MYTEST 25
struct netlink_data{ 
        struct nlmsghdr msg; 
        char data[1024]; 
};

int SendMessage(char* mes, int len)
{
	int result = 0;
	
	int sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_MYTEST);
	if(sockfd < 0){
		Log(LOG_LEVEL_ERROR, "New Socket Error!!");
		return -1;
	}

	struct sockaddr_nl clientnl;
	memset(&clientnl, 0, sizeof(struct sockaddr_nl));
	
	clientnl.nl_family = AF_NETLINK;
	clientnl.nl_pad = 0;
	clientnl.nl_pid = getpid();
	clientnl.nl_groups = 0;

	result = bind(sockfd, (struct sockaddr*)&clientnl, sizeof(struct sockaddr_nl));
	if(result < 0){
		Log(LOG_LEVEL_ERROR, "Bind Error!!");
		return -1;
	}

	struct sockaddr_nl servernl;
	memset(&servernl, 0, sizeof(struct sockaddr_nl));

	servernl.nl_family = AF_NETLINK;
	servernl.nl_pad = 0;
	servernl.nl_pid = 0;
	servernl.nl_groups = 0;

	struct nlmsghdr *nlhmsg = NULL;
	nlhmsg = (struct nlmsghdr*)malloc(NLMSG_SPACE(1024));
	if(!nlhmsg){
		Log(LOG_LEVEL_ERROR, "Malloc Mem Error!!");
		close(sockfd);
		return -1;
	}
	memset(nlhmsg, 0, sizeof(struct nlmsghdr));

	nlhmsg->nlmsg_len = NLMSG_SPACE(len);
	nlhmsg->nlmsg_pid = getpid();
	nlhmsg->nlmsg_type = 0;
	nlhmsg->nlmsg_seq = 0;
	nlhmsg->nlmsg_flags = 0;

	struct iovec iov;  
	iov.iov_base = (void *)nlhmsg;  
	iov.iov_len = nlhmsg->nlmsg_len;
	
	struct msghdr msg;
	memset(&msg, 0, sizeof(struct msghdr));
	 
	msg.msg_name = (void *)&(servernl);
	msg.msg_namelen = sizeof(servernl);
	msg.msg_iov = &iov;	
	msg.msg_iovlen = 1;	

	memcpy(NLMSG_DATA(nlhmsg), mes, len);
	
	sendmsg(sockfd, &msg, 0);

	Log(LOG_LEVEL_NORMAL, "Have Send Msg to Kernel");

	close(sockfd);

	return 0;
}

