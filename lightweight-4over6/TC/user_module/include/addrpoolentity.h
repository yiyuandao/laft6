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
#ifndef _ADDRPOOLENTITIY_H_
#define _ADDRPOOLENTITIY_H_

//Port unit minimum range
#define MINI_RANGE_VALUE	1024

typedef struct _tag_addrpoolnode_t ADDRPOOLNODE, *PADDRPOOLNODE;

typedef struct _tag_addrpoolnode_t
{
	unsigned int ip;//assign ip address
	int validbit;  /* no used bit */
	char bitmap[8];//address bit map
	PADDRPOOLNODE next;
};

typedef struct _tag_addrpoollist
{
	PADDRPOOLNODE header;//address pool link header
	PADDRPOOLNODE  curnode;//current address node 
	int length;//address pool length
}ADDRPOOLLIST, *PADDRPOOLLIST;


int InitAddrPool(PADDRPOOLLIST pAddrPoolList,
				 PADDR_POOL AddrPool);

int AllocAddrPort(PADDRPOOLLIST pList,
				  unsigned int uiRange,
				  unsigned short* usStartPort,
				  unsigned short* usStopPort,
				  unsigned int *Addr);

int DeallocAddrPort(PADDRPOOLLIST pList,
					unsigned int uiRange,
					unsigned short usStartPort,
					unsigned short usStopPort,
					unsigned long Addr);


void DestroyAddrPool(PADDRPOOLLIST pList);
#endif