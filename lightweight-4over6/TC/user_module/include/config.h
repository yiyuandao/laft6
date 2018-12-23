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
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "addrpool.h"
//configuration path
#define LAFT_CONFIG_FILE "./confs/tc.conf"

typedef struct _tag_tc_config_t
{
	unsigned int uiversion; // version
	unsigned short usmtu; // tunnel mtu
	unsigned short usPcpPort; // pcp port
	unsigned char ucLocalIPv6Addr[16];//local ipv6 address
	unsigned int ucLocalIPv4Addr;//local ipv4 address

}TCCONFIG, *PTCCONFIG;

typedef struct _tag_config_t
{
	ADDR_POOL 	  AddrPool; //address pool
	ADDR_POOL	  BakupAddrPool; //backup address pool
	unsigned short usPortMask; // port range
	//unsigned short usUserQutoa; // user qutoa
	//unsigned short usRequestQutoa; // request qutoa
	TCCONFIG tc_config; 
}CONFIG, *PCONFIG;

int LoadConfig(PCONFIG pConfig);
#endif
