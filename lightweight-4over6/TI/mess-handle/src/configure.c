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
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "configure.h"
#include "log.h"
#include "global.h"

int CmdDispatch(PCONFIG config, PLAFT_NAT_INFO natinfo, char* tok, char* value)
{
	int result = 0;
	if(strcmp(tok, "LocalIPv6Addr") == 0){
		result = inet_pton(AF_INET6, value, config->localipv6);
		if(result == 0){
			Log(LOG_LEVEL_ERROR, "Wrong LocalIPv6Addr Format!!");
			exit(-1);
		}

		inet_pton(AF_INET6, value, natinfo->IPv6LocalAddr);
	}

	if(strcmp(tok, "PhysicalComGatewayAddr") == 0){
		result = inet_pton(AF_INET6, value, config->gwipv6);
		if(result == 0){
			Log(LOG_LEVEL_ERROR, "Wrong PhysicalComGatewayAddrFormat!!");
			exit(-1);
		}
	}

	if(strcmp(tok, "VirtualComGatewayAddr") == 0){
		result = inet_pton(AF_INET6, value, config->virgwipv6);
		if(result == 0){
			Log(LOG_LEVEL_ERROR, "Wrong VirtualComGatewayAddr Format!!");
			exit(-1);
		}

		inet_pton(AF_INET6, value, natinfo->IPv6RemoteAddr);
	}

	//if(strcmp(tok, "PrefIPv6Addr") == 0){
	//	memcpy(config->prefipv6, value, sizeof(value));
	//}

	if(strcmp(tok, "PcpPort") == 0){
		if(atoi(value) < 65535){
			config->pcpport = atoi(value);
		}else{
			Log(LOG_LEVEL_ERROR, "Wrong PCP Port Set!!");
			exit(-1);
		}
	}

	if(strcmp(tok, "LogFlag") == 0){
		config->Logflag = atoi(value);
	}

	if(strcmp(tok, "DnsSrvIPv6") == 0){
		result = inet_pton(AF_INET6, value, config->dnssrvipv6);
		if(result == 0){
			Log(LOG_LEVEL_ERROR, "Wrong DnsSrvIPv6 Format!!");
			exit(-1);
		}
	}
	return 0;
}

int CmdLine(PCONFIG config, PLAFT_NAT_INFO natinfo, char* line)
{
	char* content = NULL;
	char* tok = NULL;
	char* value = NULL;

	content = line;
	while((line[strlen(content) - 1] == '\n') || (line[strlen(content) - 1]) == '\r'){
		line[strlen(content) - 1] = '\0';
	}

	tok = strtok(content, "=");
	if(tok == NULL){
		return 0;
	}
	
	value = strtok(NULL, "=");
	if(value == NULL){
		Log(LOG_LEVEL_ERROR, "Value Is Not Exist!!");
	}

	switch(*tok){
		case '#':
		case '\0':
		case '[':
			break;
		default:
			return CmdDispatch(config, natinfo, tok, value);
	}

	return 0;
}

int LoadConfig(PCONFIG config, PLAFT_NAT_INFO natinfo)
{
	FILE* fp = NULL;
	char line[256] = {0};
	int ln = 0;

	fp = fopen(CPE_CONFIG, "r");
	if(fp == NULL){
		Log(LOG_LEVEL_ERROR, "Open CPE Config File Error!!");
		return -1;
	}

	for(ln = 0 ; ; ln++){
		if(fgets(line, sizeof(line), fp) == NULL){
			fclose(fp);
			return 0;
		}

		if(CmdLine(config, natinfo, line)< 0){
			Log(LOG_LEVEL_ERROR, "Handle Config Line ERROR!!");
			return -1;
		}
	}

	fclose(fp);
	return 0;
}

