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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "addrpool.h"

static unsigned long poolprefixarray[32]=
{
   0x0,0x8,0xC,0xE,
   0xF,0xF8,0xFC,0xFE,
   0xFF,0x80FF,0xC0FF,0xE0FF,
   0xF0FF,0xF8FF,0xFCFF,0xFEFF,
   0xFFFF,0x80FFFF,0xC0FFFF,0xE0FFFF,
   0xF0FFFF,0xF8FFFF,0xFCFFFF,0xFEFFFF,
   0xFFFFFF,0x80FFFFFF,0xC0FFFFFF,0xE0FFFFFF,
   0xF0FFFFFF,0xF8FFFFFF,0xFCFFFFFF,0xFEFFFFFF
};

/** 
 * @fn    static int addAddrRange(char* tok, unsigned int *uiStartIP, unsigned int *uiEndIP)
 * @brief collect range of ipv4 address
 * 
 * @param[in] tok string
 * @param[out] uiStartIP start of ipv4 address
 * @param[out] uiEndIP 	 end of ipv4 address
 * @retval 1  success 
 * @retval -1 Failure 
 */
static int addAddrRange(char* tok, unsigned int *uiStartIP, unsigned int *uiEndIP)
{
	char* sip;
	char* sprefix;
	int prefix;
	unsigned long ip,maxip;

	sip = strtok(tok," ");
	if (sip == NULL)
		return -1;
	
	sip = strtok(NULL," ");
	if (sip == NULL)
		return -1;
	sip = strtok(sip,"/");
	if (sip == NULL)
		return -1;

	sprefix = strtok(NULL,"/");
	if (sprefix == NULL)
		return -1;

	prefix = atoi(sprefix);
	if(prefix < 0 || prefix > 32)
	{
		return -1;
	}
	if(prefix == 32)
		return -1;

    ip = inet_addr(sip);
	if(ip == INADDR_NONE)
	{
	    return -1;
	}
        
    if(prefix == 0)
    {
		*uiStartIP = *uiEndIP = ntohl(ip);
		return 1;
    }
    ip = ip&(poolprefixarray[prefix]);
    maxip = ip|(~poolprefixarray[prefix]);
	*uiStartIP = htonl(ip);	
	*uiEndIP = htonl(maxip);
	
	return 1;
}

/** 
 * @fn    static int addAddrSingle(char* tok, unsigned int *uiIP)
 * @brief collect single of ipv4 address
 * 
 * @param[in] tok string
 * @param[out] uiIP start of ipv4 address
 * @retval 1  success 
 * @retval -1 Failure 
 */
static int addAddrSingle(char* tok, unsigned int *uiStartIP, unsigned int *uiEndIP)
{
	char* sip;
    unsigned long ip;
    
    sip = strtok(tok," ");
	sip = strtok(NULL," ");

	ip = inet_addr(sip);
	if(ip == INADDR_NONE)
		return -1;
	
	*uiStartIP = *uiEndIP = htonl(ip);
	return 1;    
}


/** 
 * @fn    static int addAddrLimits(char* tok, unsigned int *uiStartIP, unsigned int *uiEndIP)
 * @brief collect limit of ipv4 address
 * 
 * @param[in] tok string
 * @param[out] uiStartIP start of ipv4 address
 * @param[out] uiEndIP 	 end of ipv4 address
 * @retval 1  success 
 * @retval -1 Failure 
 */
static int addAddrLimits(char* tok, unsigned int *uiStartIP, unsigned int *uiEndIP)
{
	char* sip;
    unsigned long minip,maxip;    

    sip = strtok(tok," ");
	sip = strtok(NULL," ");

	char* startip = strtok(sip,"-");
	char* endip = strtok(NULL,"-");

    minip = inet_addr(startip);
	maxip = inet_addr(endip);

	if(minip == INADDR_NONE || maxip == INADDR_NONE)
 	{
 	    return -1;
 	}

    *uiStartIP = htonl(minip);
    *uiEndIP = htonl(maxip);
	
   	return 1;
}


/** 
 * @fn   static int parsePortMask(char* tok, unsigned short usPortMask)
 * @brief collect port mask
 * 
 * @param[in] tok string
 * @param[out] usPortMask port range
 * @retval 1  success 
 * @retval -1 Failure 
 */
static int parsePortMask(char* tok, unsigned short *usPortMask)
{
	char* pucport = NULL;
	int iRet = 0;
	
	pucport = strtok(tok," ");
	pucport = strtok(NULL," ");
	
	iRet = sscanf(pucport, "%x", usPortMask);
	if (iRet < 0)
		return -1;

	if (*usPortMask > 0 && *usPortMask <= 65535)
	{
		return 1;
	}
	else
	{
		return -1;
	}
}

static int parseUserQutoa(char* tok, unsigned int *usUserQutoa)
{
	char* pUserQutoa = NULL;
	int iRet = 0;
	
	pUserQutoa = strtok(tok," ");
	pUserQutoa = strtok(NULL," ");
	
	if ( (iRet = sscanf(pUserQutoa, "%u", usUserQutoa)) < 0)
	{
		perror("sscanf:");
		return -1;
	}

	if (*usUserQutoa > 0 && *usUserQutoa <= 65535)
	{
		return 1;
	}
	else
	{
		printf("uqutoa < 0\n");
		return -1;
	}
}

static int parseRequestQutoa(char* tok, unsigned int *usRequestQutoa)
{
	char* pReqQutoa = NULL;
	int iRet = 0;
	
	pReqQutoa = strtok(tok," ");
	pReqQutoa = strtok(NULL," ");
	
	if ( (iRet = sscanf(pReqQutoa, "%u", usRequestQutoa)) < 0)
	{
		perror("1sscanf:");
		return -1;
	}

	if (*usRequestQutoa > 0 && *usRequestQutoa <= 65535)
	{
		return 1;
	}
	else
	{
		printf("rqutoa < 0\n");
		return -1;
	}
}
/** 
 * @fn    int LoadAddrPoolConfig(PADDR_POOL pAddrPool, char *FileName)
 * @brief load address pool configurate file
 * 
 * @param[out] pAddrPool address pool context
 * @param[in] FileName configurate file name
 * @retval 1  success 
 * @retval -1 Failure 
 */
int LoadAddrPoolConfig(PADDR_POOL pAddrPool, char *FileName)
{
	FILE *fp;
	char line[256];
	int ln;

	fp = fopen(FileName, "r");
	if(fp == NULL){
		return -1;
	}

	memset(line,0,sizeof(line));

	for(ln =1; ;ln++){
		if(fgets(line,sizeof(line),fp) == NULL){
			fclose(fp);
			return 0;
		}
		if(HandleConf(line, pAddrPool) < 0)
		{
			fclose(fp);
			return -1;
		}
	}

	fclose(fp);

	return 1;
}


/** 
 * @fn    int HandleConf(char* Line, PADDR_POOL pAddrPool)
 * @brief filter invaild character
 * 
 * @param[in] Line string
 * @param[out] pAddrPool address of pool
 * @retval 1  success 
 * @retval -1 Failure 
 */
int HandleConf(char* Line, PADDR_POOL pAddrPool)
{
	char *l, *tok;
	unsigned int len = 0;
	
	l = Line;
	
	len = strlen(l);
	if (len < 0)
	{
		return -1;
	}
	
	len = len - 1;
	
	while((Line[len] == '\n')||(Line[len] == '\r')){
		Line[len] = '\0';
	}

	tok = strtok(l,"\t");
	if(tok == NULL){
		return -1;
	}

	switch(*tok){
		case '#':
		case '\0':
			return 1;
		default:
			return HandleCmd(tok, pAddrPool);
	}
	return 1;
}


/** 
 * @fn    int HandleCmd(char* tok, PADDR_POOL pAddrPool)
 * @brief The configuration files of the operation of field
 * 
 * @param[in] tok string
 * @param[out]address of pool context
 * @retval 1  success 
 * @retval -1 Failure 
 */
int HandleCmd(char* tok, PADDR_POOL pAddrPool)
{
	int c;
	int ret = 0;
	
	c = tolower(*tok);
	switch(c){
		case 'r':
			return addAddrRange(tok, &pAddrPool->uiStartIp, &pAddrPool->uiEndIp);
		case 's':
			return addAddrSingle(tok,&pAddrPool->uiStartIp, &pAddrPool->uiEndIp);
		case 'l':
			return addAddrLimits(tok,&pAddrPool->uiStartIp, &pAddrPool->uiEndIp);
		case 'p':
			return parsePortMask(tok, &pAddrPool->usPortMask); 
		case 'u':
				return   parseUserQutoa(tok, &pAddrPool->usUserQutoa); 
		case 'q':
			return parseRequestQutoa(tok, &pAddrPool->usRequestQutoa); 
		default:
			return -1;
	}
	return 1;
}


