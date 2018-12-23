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

#include <unistd.h>
#include "dnsproxy.h"
#include "thrmgr.h"

int dns_start6(void)
{
	int serverfd = 0;
	char DataBuf[BUF_SIZ] = {0};
	struct sockaddr_in6 locaddr;
	struct sockaddr_in6 clientaddr;
	struct Dns6Data* param6;
	socklen_t infolen = 0;
	int result = 0;
	threadpool_t *thpool = NULL;
	printf("dns start6 start......\n");

	//init thread pool 
	thpool = thrmgr_new(6, 30*60, dns_ops6);
	if (thpool == NULL)
	{
		Log(LOG_LEVEL_ERROR, "thrmgr_new  Error!!");
		return -1;
	}

	serverfd=socket(AF_INET6, SOCK_DGRAM, 0);
	if(serverfd < 0){
		perror("socket error:");
		Log(LOG_LEVEL_ERROR, "Socket Create Error!!");
		return -1;
	}

	memset(&locaddr, 0, sizeof(struct sockaddr_in6));
	locaddr.sin6_family=AF_INET6;
	locaddr.sin6_addr=in6addr_any;
	locaddr.sin6_port=htons((u_short)53);

	if(bind(serverfd, (struct sockaddr*)&locaddr, sizeof(locaddr))!=0){
		perror("bind error:");
		Log(LOG_LEVEL_ERROR, "Socket Bind Error!!");
		close(serverfd);
		return -1;
	}

	memset(&clientaddr, 0, sizeof(struct sockaddr_in6));
	infolen = sizeof(clientaddr);
	while(1){
		result = recvfrom(serverfd, DataBuf, sizeof(DataBuf), 0, (struct sockaddr*)&clientaddr, &infolen);
		if ((result > 0) && (result <= BUF_SIZ)){
			if(result < DNS_HDR_LEN){
				continue;
			}

			param6 = Dns6Data_Alloc(DataBuf, result, serverfd, &clientaddr);
			if(param6){
				if (!thrmgr_dispatch(thpool, (void *)param6))
				{
					Log(LOG_LEVEL_ERROR, "thrmgr_dispatch  error......");
					Dns6Data_Free(param6);
				}
			}
		}else if (result < 0){
			Log(LOG_LEVEL_WARN, "Recv from Client Error!!");
			continue;
		}
	}

	thrmgr_destroy(thpool);

	return 0;
}

struct Dns6Data* Dns6Data_Alloc(char *buf, int len, int sockfd, struct sockaddr_in6* addr)
{
	struct Dns6Data *data ;
	if((!buf) || (!len) || (!addr)){
		Log(LOG_LEVEL_ERROR, "Dns6Data_Alloc Param Is Not Correct!!");
		return NULL;
	}

	data = (struct Dns6Data*)malloc(sizeof(struct Dns6Data));
	if(data){
		data->buf =(char*)malloc(len);
		if(data->buf){
			memcpy(data->buf, buf, len);
			data->len = len;
			data->sockfd = sockfd;
			memcpy(&data->addr, addr, sizeof(struct sockaddr_in6));
		}else{
			free(data);
			data = NULL;
		}
	}

	return data;
}

void Dns6Data_Free(struct Dns6Data *data)
{
	if(data){
		free(data->buf);
		free(data);
	}
}

void dns_ops6(void* data)
{
	char DataBuf[BUF_SIZ] = {0};
	char RecvBuf[BUF_SIZ] = {0};

	int len = 0;
	socklen_t remotelen = 0;
	int ret = 0;
	int dnsforward = 0;
	int data_buf_len = 0;
	char domainname[128] = {0};
	unsigned short queryType = 0;
	int queryLen = 0;

	struct sockaddr_in6 client, remote, temp;
	struct timeval timeout;
	struct DnsHeader* pHeaderB;

	fd_set fset;

	unsigned char Rcode_Cover = 0x0F, Rcode;

//	Log(LOG_LEVEL_NORMAL, "Enter dns_ops6()");
	memset(&client, 0, sizeof(struct sockaddr_in6));
	memset(&remote, 0, sizeof(struct sockaddr_in6));

	len =( (struct Dns6Data*)data)->len;
	data_buf_len = ( (struct Dns6Data*)data)->len;
	if(len > BUF_SIZ){
		Log(LOG_LEVEL_ERROR, "tooo big packe: %d", len);
		Dns6Data_Free(data);
		return ;
	}

	memcpy(DataBuf, ( (struct Dns6Data*)data)->buf, ( (struct Dns6Data*)data)->len);
	memcpy(&client, &(( (struct Dns6Data*)data)->addr), sizeof(struct sockaddr_in6));
	dnsforward = socket(AF_INET6, SOCK_DGRAM, 0);
	if(dnsforward <0 ){
		Log(LOG_LEVEL_ERROR, "can't create socket");
		perror("socket()");
		Dns6Data_Free(data);
		return ;
	}

	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	FD_ZERO(&fset);
	FD_SET(dnsforward,&fset);

	remote.sin6_family = AF_INET6;
	memcpy(&remote.sin6_addr.s6_addr, gNatConfig.DnsSrvIpv6, sizeof(gNatConfig.DnsSrvIpv6));
	remote.sin6_port = htons((u_short)53);
	remotelen = sizeof(struct sockaddr_in6);

	len =sendto(dnsforward, DataBuf, data_buf_len, 0, (struct sockaddr*)&remote, remotelen);
	if(len <= 0){
		Log(LOG_LEVEL_ERROR, "sendto error");
		close(dnsforward);
		Dns6Data_Free(data);
		return ;
	}

	ret = select(dnsforward + 1, &fset, NULL, NULL, &timeout);
	if(ret <= 0){

		close(dnsforward);
		Dns6Data_Free(data);
		return;
	}

	len = recvfrom(dnsforward, RecvBuf, sizeof(RecvBuf), 0, (struct sockaddr*)&temp, &remotelen);
	if(len <= 0){
		Log(LOG_LEVEL_ERROR, "recvfrom error");
		close(dnsforward);
		Dns6Data_Free(data);
		return;
	}
	if (checkdns((struct DnsHeader *)DataBuf) == 0)
	{
		Log(LOG_LEVEL_ERROR, "invaild  dns packet!");
		Dns6Data_Free(data);
		close(dnsforward);
		return;
	}

	queryType = checkquery(DataBuf, data_buf_len, domainname, &queryLen);

	if(queryType == DNSA){
		len = sendto(((struct Dns6Data*)data)->sockfd, RecvBuf, len, 0, (struct sockaddr*)&client, sizeof(client));
		if(len <= 0){
			perror("DNSA: sendto()");
			Log(LOG_LEVEL_ERROR, "DNSA: Send To Client Data Error!!");
		}
	}else if(queryType == DNSAAAA){
		/* when query is AAAA */
		memcpy(&Rcode, DataBuf + 3, 1);
		pHeaderB = (struct DnsHeader*)RecvBuf;

		if(((Rcode & Rcode_Cover) == RECORD_EXIST) && (ntohs(pHeaderB->answers) >0)){
			len = sendto(((struct Dns6Data*)data)->sockfd, RecvBuf, len, 0, (struct sockaddr*)&client, sizeof(client));
			if(len <= 0){
				perror("DNS4A: sendto()");
				Log(LOG_LEVEL_ERROR, "DNS4A Send To Client Data Error!!");
			}
		}
	}

//	Log(LOG_LEVEL_NORMAL, "End dns_ops6()");
	close(dnsforward);
	Dns6Data_Free(data);
	return ;
}

unsigned short checkquery(char *buf, int len, char *domainname, int *namelen)
{
	unsigned short queryType = 0;
	int dnlen = countlenofquery(buf + DNS_HDR_LEN, len);

	memcpy(&queryType, buf + DNS_HDR_LEN + dnlen, 2);
	queryType = ntohs(queryType);
	memcpy(domainname, buf + DNS_HDR_LEN, dnlen);
	*namelen = dnlen;
	return queryType;
}

int countlenofquery(const char* buf, int len)
{
	int i = 0, length = 0;
	while((i < len)  && ((unsigned char)buf[i] !=0) ){
		length += (unsigned char)buf[i] + 1;
		i += (unsigned char)buf[i]+1;
	}

	return length + 1;
}


unsigned char checkdns(struct DnsHeader *dnshdr)
{
		if ((htons(dnshdr->flags) == 0x0100) && (htons(dnshdr->queries) == 1))
					return 1;
			return 0;
}


