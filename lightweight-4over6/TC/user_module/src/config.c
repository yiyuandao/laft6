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

#include <netinet/in.h> 
#include <arpa/inet.h> 
#include "config.h"
#include "addrpool.h"
#include "log.h"
#include "opcode.h"

/** 
 * @fn    int CmdAddress(PTCCONFIG pTcConfig, char *tok)
 * @brief handle local ipv6 address
 * 
 * @param[out] pTcConfig configuration context
 * @param[in] tok string
 * 
 * @retval 1  success 
 * @retval -1 Failure 
 */
static int CmdAddress(PTCCONFIG pTcConfig, char *tok)
{

	char *ntok;

	ntok = strtok(NULL, " \t");
	if (ntok == NULL)
	{
		Log(LOG_LEVEL_NORMAL, "ntok == NULL");
		goto usage;
	}
	
	if (strtok(NULL, " \t") != NULL)
	{
		Log(LOG_LEVEL_NORMAL, "goto usage0");
		goto usage;
	}
	
	if (strcasecmp(tok, "endpoint") == 0)
	{
		if (inet_pton(AF_INET6, ntok, pTcConfig->ucLocalIPv6Addr) != 1)
		{
			Log(LOG_LEVEL_NORMAL, "format ntok: %s", ntok);
			goto usage;
		}
		return 1;
	}
	else
	{
		Log(LOG_LEVEL_ERROR, "strcasecmp failed");
	}
    usage:
	
	return -1;
}

static int CmdLocalIp(PTCCONFIG pTcConfig, char *tok)
{

	char *ntok;

	ntok = strtok(NULL, " \t");
	if (ntok == NULL)
	{
		Log(LOG_LEVEL_NORMAL, "ntok == NULL");
		goto usage;
	}
	
	if (strtok(NULL, " \t") != NULL)
	{
		Log(LOG_LEVEL_NORMAL, "goto usage0");
		goto usage;
	}
	
	if (strcasecmp(tok, "localipv4") == 0)
	{
		if (inet_pton(AF_INET, ntok, &pTcConfig->ucLocalIPv4Addr) != 1)
		{
			Log(LOG_LEVEL_NORMAL, "format ntok: %s", ntok);
			goto usage;
		}

		pTcConfig->ucLocalIPv4Addr = ntohl(pTcConfig->ucLocalIPv4Addr);
		
		return 1;
	}
	else
	{
		Log(LOG_LEVEL_ERROR, "strcasecmp failed");
	}
    usage:
	
	return -1;
}
static int CmdVersion(PTCCONFIG pTcConfig, char *tok)
{
	char *ntok;
	int version = 0;

	ntok = strtok(NULL, " \t");
	if (ntok == NULL)
	{
		Log(LOG_LEVEL_NORMAL, "ntok == NULL");
		goto usage;
	}
	
	if (strtok(NULL, " \t") != NULL)
	{
		Log(LOG_LEVEL_NORMAL, "goto usage1");
		goto usage;
	}

	if (strcasecmp(tok, "version") == 0)
	{
		version = atoi(ntok);
		pTcConfig->uiversion = version;

		return 1;
	}

    usage:
		return -1;
}

static int CmdMtu(PTCCONFIG pTcConfig, char *tok)
{

	char *ntok;
	unsigned short mtu = 0;

	ntok = strtok(NULL, " \t");
	if (ntok == NULL)
	{
		Log(LOG_LEVEL_NORMAL, "ntok == NULL");
		goto usage;
	}
	
	if (strtok(NULL, " \t") != NULL)
	{
		Log(LOG_LEVEL_NORMAL, "goto usage2");
		goto usage;
	}

	if (strcasecmp(tok, "mtu") == 0)
	{
		mtu = atoi(ntok);
		if (mtu > 0)
		{
			pTcConfig->usmtu = mtu;
			return 1;
		}
		return -1;
	}

    usage:
		return -1;
}

static int CmdPort(PTCCONFIG pTcConfig, char *tok)
{

	char *ntok;
	unsigned short port = 0;

	ntok = strtok(NULL, " \t");
	if (ntok == NULL)
	{
		Log(LOG_LEVEL_NORMAL, "ntok == NULL");
		goto usage;
	}
	
	if (strtok(NULL, " \t") != NULL)
	{
		Log(LOG_LEVEL_NORMAL, "goto usage2");
		goto usage;
	}

	if (strcasecmp(tok, "port") == 0)
	{
		port = atoi(ntok);
		if (port > 0)
		{
			pTcConfig->usPcpPort = port;
			return 1;
		}
		return -1;
	}

    usage:
		return -1;
}
/** 
 * @fn    int HandleLaftCmd(char* tok, PTCCONFIG pTcConfig)
 * @brief filter string
 * 
 * @param[in] tok string
 * @param[out] pTcConfig configuration context
 * 
 * @retval 1  success 
 * @retval -1 Failure 
 */
static int HandleLaftCmd(char* tok, PTCCONFIG pTcConfig)
{
	int c;
	c = tolower(*tok);
	switch(c){
		case 'e':
			return CmdAddress(pTcConfig, tok); 
		case 'v':
			return CmdVersion(pTcConfig, tok); 
		case 'm':
			return CmdMtu(pTcConfig, tok); 
		case 'p':
			return CmdPort(pTcConfig, tok); 
		case 'l':
			return CmdLocalIp(pTcConfig, tok); 
		default:

			Log(LOG_LEVEL_ERROR, "in default");
			return -1;
	}
	return 1;
}

/** 
 * @fn    static int HandleLaftConf(char* Line, PTCCONFIG pTcConfig)
 * @brief handle string 
 * 
 * @param[in] Line string
 * @param[out] pConfig configuration context
 * 
 * @retval 1  success 
 * @retval -1 Failure 
 */
static int HandleLaftConf(char* Line, PTCCONFIG pTcConfig)
{
	char *l, *tok;
	unsigned int len = 0;
	
	l = Line;
	
	len = strlen(l);
	if (len < 0)
	{
		Log(LOG_LEVEL_ERROR, "len < 0");
		return -1;
	}
	

	while((Line[strlen(l) - 1] == '\n')||(Line[strlen(l) - 1] == '\r')){
		Line[strlen(l) - 1] = '\0';
	}

	if (l[0] == '\0')
    {
     	return 1;
	}
	 
	tok = strtok(l, "\t");
	if(tok == NULL){
		return -1;
	}

	switch(*tok){
		case '#':
		case '\0':
			return 1;
		default:
			return HandleLaftCmd(tok, pTcConfig);
	}
	return -1;
}

/** 
 * @fn    static int LoadLaftConfig(PTCCONFIG pTcConfig, char *FileName)
 * @brief handle string 
 * 
 * @param[in] FileName file name
 * @param[out] pTcConfig configuration context
 * 
 * @retval 0  success 
 * @retval -1 Failure 
 */
static int LoadLaftConfig(PTCCONFIG pTcConfig, char *FileName)
{
	FILE *fp;
	char line[256];
	int ln;

	fp = fopen(FileName, "r");
	if(fp == NULL){
		Log(LOG_LEVEL_ERROR, "!!!!!!load laft config error");
		perror("LoadLaftConfig");
		return -1;
	}

	memset(line,0,sizeof(line));

	for(ln = 1; ;ln++)
	{
		if(fgets(line,sizeof(line),fp) == NULL){
			fclose(fp);
			return 0;
		}
		if(HandleLaftConf(line, pTcConfig) < 0)
		{
			Log(LOG_LEVEL_ERROR, "parse laft conf error");
			fclose(fp);
			return -1;
		}
	}

	fclose(fp);

	return 0;
}


/** 
 * @fn    int LoadConfig(PCONFIG pConfig)
 * @brief load configuration file
 * 
 * @param[out] pConfig configuration context
 * 
 * @retval 1  success 
 * @retval -1 Failure 
 */
int LoadConfig(PCONFIG pConfig)
{
	int iRet = 0;

	//load laft configuration file
	iRet = LoadLaftConfig(&pConfig->tc_config, LAFT_CONFIG_FILE);
	if (iRet < 0)
	{
		Log(LOG_LEVEL_ERROR, "laft config error");
		return -1;
	}

	//load address pool configuration file
	iRet = LoadAddrPoolConfig(&pConfig->AddrPool, ADDR_POOL_FILE);
	if (iRet < 0)
	{
		Log(LOG_LEVEL_ERROR, "addr config error");
		return -1;
	}

	//load address backup pool configuration file
	iRet = LoadAddrPoolConfig(&pConfig->BakupAddrPool, BAK_ADDR_POOL_FILE);
	if (iRet < 0)
	{
		Log(LOG_LEVEL_ERROR, "bak addr config error");
		return -1;
	}

	// load opcode and option configuration file
	iRet = load_opcode_file(OPCODE_FILE);
	if (iRet < 0)
	{
		Log(LOG_LEVEL_ERROR, "opcode config error");
		return -1;
	}
	return 1;
}

