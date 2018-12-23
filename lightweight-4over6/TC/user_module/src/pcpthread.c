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
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "thrmgr.h"
#include "log.h"
#include "newpcpmsgdef.h"
#include "global.h"

typedef struct _tag_pcpdat
{
	int sockfd; // socket file descipte for sending data
	unsigned int pcplen; // pcp message length
	struct sockaddr_in6 client; // clieng address
	unsigned char *dat;    // pcp request
}PcpDat_t, *pPcpDat_t;

/** 
 * @fn  pcp_open
 * @brief handl pcp request
 * 
 * @param[in] param thread args(pcp request msg and sockfd)
 * 
 * @retval void
 */
void pcp_open(void *param)
{
	//pthread_detach(pthread_self());
	pPcpDat_t pPcpDat = (pPcpDat_t)param;

	//Log(LOG_LEVEL_NORMAL, "msgrecv msglen:%d", pPcpDat->pcplen);

	handlePcpRequest(pPcpDat->dat, pPcpDat->pcplen, pPcpDat->sockfd, &pPcpDat->client);

	free(pPcpDat->dat);
	pPcpDat->dat = NULL;
	free(pPcpDat);
	pPcpDat = NULL;

	return;

}

/** 
 * @fn  pcp_start
 * @brief create socket, bind, recv pcp request and dispatch pcp request
 * 
 * @retval void
 */
void pcp_start(void)
{
	struct sockaddr_in6 sa;
	struct sockaddr_in6 client;
	socklen_t length = sizeof(client);
	int pcpfd  = 0;
	int pcplen = 0;
	int on;
	int writebytes = 0;
	threadpool_t  *threadpool = NULL; 	
	char recvbuf[2048] = {0};

	//init thread pool
	threadpool = thrmgr_new(2, 60 * 60, pcp_open);
	if (threadpool == NULL)
	{
		return;
	}

	pcpfd = socket(PF_INET6, SOCK_DGRAM, 0);
	if (pcpfd < 0) 
	{
		Log(LOG_LEVEL_ERROR, "socket(inet6): %s", strerror(errno));
		goto END;

	}

	memset(&sa, 0, sizeof(sa));
	sa.sin6_family = AF_INET6;
	sa.sin6_addr = in6addr_any;
	sa.sin6_port = htons((uint16_t) GlobalCtx.Config.tc_config.usPcpPort );
	on = 1;

	// set reuseaddr option
	if (setsockopt(pcpfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
	{
		Log(LOG_LEVEL_ERROR, "SO_REUSEADDR(inet6): %s", strerror(errno));
		(void)close(pcpfd);
		goto END;
	}

	if (bind(pcpfd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
	{
		Log(LOG_LEVEL_ERROR, "bind(inet6): %s", strerror(errno));
		(void) close(pcpfd);
		goto END;
	}

	while (1)
	{
		pcplen = recvfrom(pcpfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr*)&client, &length);
		if (pcplen <= 0)
		{
			continue;
		}

		do 
		{
			pPcpDat_t pPcpDat= NULL;

			pPcpDat = malloc(sizeof(*pPcpDat));
			if (pPcpDat == NULL)
			{
				Log(LOG_LEVEL_ERROR, "in while(1) no mem to alloc");
				break;
			}

			pPcpDat->sockfd = pcpfd;
			memcpy(&pPcpDat->client, &client, sizeof(client));
			pPcpDat->pcplen = pcplen;

			pPcpDat->dat = malloc(pcplen);
			if (pPcpDat->dat == NULL)
			{
				free(pPcpDat);
				break;
			}

			memcpy(pPcpDat->dat, recvbuf, pcplen);

			if (thrmgr_dispatch(threadpool, pPcpDat) < 0)
			{
				Log(LOG_LEVEL_ERROR, "thrmgr_dispatch failed!!");
			}
		}
		while (0);

	}
END:
	thrmgr_destroy(threadpool);
	return; 
}
