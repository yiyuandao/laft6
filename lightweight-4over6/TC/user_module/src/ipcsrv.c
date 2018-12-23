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
#include "log.h"
#include <errno.h>
#include "ipcsrv.h"
#include "thrmgr.h"

typedef struct _tag_ipcsrv_ctx
{
	unsigned char 		af_type;			
	int 				ipc_socket;
	threadpool_t		*pthreadpool;
	pthread_t 			ipcsrvthreadid;
	int 				sockpair[2];
}IPCSRV_CTX, *PIPCSRV_CTX;


#define SOCK_PAIR_READ 		0
#define SOCK_PAIR_WRITE 	1
#define MAX_BUF_SIZE 		1500

PIPCSRV_CTX pipcsrvctx = NULL;

static void *ipc_server_message_func(void *param)
{	
	fd_set     fdR;       
	struct     timeval   timeout  = {1000, 0};
	int iret = 0;
	int maxfd = 0;
	char recvbuf[MAX_BUF_SIZE] = {0};
	struct sockaddr_in client_addr;
	struct sockaddr_in6 client_addr6;
	socklen_t addrlen = 0;
	PIPCTASKCTX pipctaskctx = NULL;
		
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, pipcsrvctx->sockpair) < 0) 
    	  return NULL;

	while (1) {
		FD_ZERO(&fdR);       
		FD_SET(pipcsrvctx->sockpair[SOCK_PAIR_READ], &fdR);
		FD_SET(pipcsrvctx->ipc_socket, &fdR);
		maxfd = pipcsrvctx->ipc_socket > pipcsrvctx->sockpair[SOCK_PAIR_READ]?(pipcsrvctx->ipc_socket + 1):(pipcsrvctx->sockpair[SOCK_PAIR_READ] + 1);
		iret = select(maxfd, &fdR, NULL, NULL, &timeout);
		if (iret > 0) {
			if (FD_ISSET(pipcsrvctx->sockpair[SOCK_PAIR_READ], &fdR)){
				break;
			}
			if (FD_ISSET(pipcsrvctx->ipc_socket, &fdR)){
				if (pipcsrvctx->af_type == AF_INET) {
					addrlen = sizeof(client_addr);
					iret = recvfrom(pipcsrvctx->ipc_socket,
						recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&client_addr, &addrlen);
					if (iret > 0){
						pipctaskctx = (PIPCTASKCTX)malloc(sizeof(IPCTASKCTX) - sizeof(char) + iret);
						pipctaskctx->msglen = iret;
						memcpy(&pipctaskctx->addr.client_addr, &client_addr, addrlen);
						memcpy(&pipctaskctx->msgcontext[0], recvbuf, iret);
						thrmgr_dispatch(pipcsrvctx->pthreadpool, pipctaskctx);
					}
				}else if (pipcsrvctx->af_type == AF_INET6) {
					addrlen = sizeof(client_addr6);
					iret = recvfrom(pipcsrvctx->ipc_socket,
						recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&client_addr6, &addrlen);
					if (iret > 0) {
						pipctaskctx = (PIPCTASKCTX)malloc(sizeof(IPCTASKCTX) - sizeof(char) + iret);
						pipctaskctx->msglen = iret;
						memcpy(&pipctaskctx->addr.client_addr6, &client_addr6, addrlen);
						memcpy(&pipctaskctx->msgcontext[0], recvbuf, iret);
						thrmgr_dispatch(pipcsrvctx->pthreadpool, pipctaskctx);
					}
				}
			}
		}
		else if (iret < 0){
			break;
		}
	}
	
	Log(LOG_LEVEL_NORMAL, "ipc_server_message_func exit!");
	ipc_server_clear();
	return NULL;
}


int ipc_server_init(unsigned char af_type, IPC_MESSAGE_HANDLE ipcfunc)
{
	int err = 0;
	struct sockaddr_in6 local_addr6;
	struct sockaddr_in	local_addr;
	int addrlen = 0;
	
	pipcsrvctx = (PIPCSRV_CTX)malloc(sizeof(IPCSRV_CTX));
	if (pipcsrvctx == NULL){
		err = 1;
		goto ERR_LABLE;
	}
	
	memset(pipcsrvctx, 0, sizeof(IPCSRV_CTX));
	pipcsrvctx->af_type = af_type;
	pipcsrvctx->ipc_socket = socket(af_type, SOCK_DGRAM, 0);
	if (pipcsrvctx->ipc_socket == -1) {
		err = 1;
		goto ERR_LABLE;
	}

	if (af_type == AF_INET6) {
		memset(&local_addr6, 0, sizeof(local_addr6));
		local_addr6.sin6_family = af_type;
		local_addr6.sin6_port = htons(IPC_SRV_PORT);
		local_addr6.sin6_addr = in6addr_any;
		addrlen = sizeof(local_addr6);
		if (bind(pipcsrvctx->ipc_socket, (struct sockaddr *)&local_addr6, addrlen) != 0) {
			err = 1;
			goto ERR_LABLE;
		}
	} else if (af_type == AF_INET) {
		memset(&local_addr, 0, sizeof(local_addr));
		local_addr.sin_family = af_type;
		local_addr.sin_port = htons(IPC_SRV_PORT);
		local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addrlen = sizeof(local_addr);
		if (bind(pipcsrvctx->ipc_socket, (struct sockaddr *)&local_addr, addrlen) != 0) {
			err = 1;
			goto ERR_LABLE;
		}
	}
	
	pipcsrvctx->pthreadpool = thrmgr_new(2, 1000, ipcfunc);
	if (pipcsrvctx->pthreadpool == NULL) {
		err = 1;
		goto ERR_LABLE;
	}

ERR_LABLE:	
	if (err == 1) {
		if (pipcsrvctx->ipc_socket) {
			close(pipcsrvctx->ipc_socket);
			pipcsrvctx->ipc_socket = 0;
		}
		if (pipcsrvctx) {
			free(pipcsrvctx);
			pipcsrvctx = NULL;
		}
		return -1;
	}
	return 1;
}

void ipc_server_clear(void)
{
	if (pipcsrvctx)
	{
		if (pipcsrvctx->ipc_socket > 0)
			close (pipcsrvctx->ipc_socket);
		if (pipcsrvctx->sockpair[SOCK_PAIR_READ] > 0)
			close(pipcsrvctx->sockpair[SOCK_PAIR_READ]);
		if (pipcsrvctx->sockpair[SOCK_PAIR_WRITE] > 0)
			close(pipcsrvctx->sockpair[SOCK_PAIR_WRITE]);
		thrmgr_destroy(pipcsrvctx->pthreadpool);
		pthread_cancel(pipcsrvctx->ipcsrvthreadid);
		free(pipcsrvctx);
		pipcsrvctx = NULL;
	}
}


void ipc_server_start(void)
{	
	if (pipcsrvctx)
		pthread_create(&pipcsrvctx->ipcsrvthreadid, 
			NULL,
			(void *)ipc_server_message_func, 
			NULL);
}

void ipc_server_stop(void)
{
	if (pipcsrvctx && pipcsrvctx->sockpair[SOCK_PAIR_WRITE] > 0) {
		write(pipcsrvctx->sockpair[SOCK_PAIR_WRITE], "exit", 5);
	}
}


int ipc_server_send(unsigned char *szbuf,
	unsigned int bufsize,
	struct sockaddr *cltaddr,
	unsigned int addrlen)
{
	int ret = 0;
	
	if (pipcsrvctx)
		ret = sendto(pipcsrvctx->ipc_socket, 
		szbuf,
		bufsize, 0, 
		cltaddr, 
		addrlen);

	return ret;
}


