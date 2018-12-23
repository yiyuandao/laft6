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
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <asm/types.h>
#include <linux/socket.h>
#include <errno.h>
//
//@inbuf:输入缓冲区
//@inbufsize:输入缓冲区的大小
//@outbuf:输出缓冲区
//@outbufsize:输出缓冲区的大小
//@to:目标地址
//@iswait:发送后是否需要等待返回数据 1等待 0不等待 等待3秒
//返回值:实际接收或者发送的大小
//
int ipc_clt_send(unsigned char *inbuf, unsigned int inbufsize, 
				 unsigned char *outbuf, unsigned int outbufsize, 
					struct sockaddr* to, 
						int iswait)
{
	int ipc_sock = -1;
	int ret = -1;
	struct timeval  timeout = {3, 0};
	struct sockaddr_in6 Srv6Sa;
	struct sockaddr_in SrvSa;


	socklen_t addrlen6 = sizeof(Srv6Sa); 
	socklen_t addrlen = sizeof(SrvSa);

	ipc_sock = socket(to->sa_family, SOCK_DGRAM, 0);	
	if (ipc_sock < 0)
	{
		return -1;
	}

	ret = setsockopt(ipc_sock,
		SOL_SOCKET,
		SO_RCVTIMEO,
		(char*)&timeout,
		sizeof(timeout)); 
	if(ret < 0)
	{
		printf("setsockopt error!\n");
		close (ipc_sock);
		return ret;
	}

	if (AF_INET == to->sa_family)
	{
		ret = sendto(ipc_sock, (char*)inbuf, inbufsize, 0, to, sizeof(struct sockaddr_in));
		if (ret < 0)
		{
			close (ipc_sock);
			return -1;
		}

		if (iswait)
		{
			ret = recvfrom(ipc_sock, (char *)outbuf, outbufsize, 0, (struct sockaddr *)&SrvSa, &addrlen);
			if (ret < 0)
			{
				close (ipc_sock);
				return -1;
			}	
		}
	}
	else if (AF_INET6 == to->sa_family)
	{
		ret = sendto(ipc_sock, (char *)inbuf, inbufsize, 0, to, sizeof(struct sockaddr_in6));
		if (ret < 0)
		{
			close (ipc_sock);
			return -1;
		}
		if (iswait)
		{
			ret = recvfrom(ipc_sock, (char *)outbuf, outbufsize, 0, (struct sockaddr *)&Srv6Sa, &addrlen6);
			if (ret < 0)
			{
				close (ipc_sock);
				return -1;
			}	
		}	
	}

	close (ipc_sock);
	return ret;
}
