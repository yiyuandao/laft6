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
#include <stdlib.h>
#include <string.h>
#include "addrpool.h"
#include "addrpoolentity.h"
#include "portmap.h"

/** 
* @fn    static PADDRPOOLNODE AllocAddrPoolNode(unsigned int addr)
* @brief Distribution address pool node
* 
* @param[in] address
* @retval PADDRPOOLNODE  Address pool node context 
* @retval NULL Failure 
*/
static PADDRPOOLNODE AllocAddrPoolNode(unsigned int addr)
{
	PADDRPOOLNODE pAddrPoolNode = NULL;

	pAddrPoolNode = (PADDRPOOLNODE)malloc(sizeof(ADDRPOOLNODE));
	if (pAddrPoolNode == NULL)
		return NULL;

	memset(pAddrPoolNode, 0, sizeof(ADDRPOOLNODE));

	pAddrPoolNode->ip = addr;

	return pAddrPoolNode;
}

/** 
* @fn    static int AddAddrPoolNode(PADDRPOOLLIST pAddrNodeList, PADDRPOOLNODE pAddrNode)
* @brief Create address list of the pool
* 
* @param[in] pAddrNodeList address list of the pool 
* @param[in] pAddrNode address node of the pool 
* @retval -1 Failure 
* @retval  1 Success 
*/
static int AddAddrPoolNode(PADDRPOOLLIST pAddrNodeList, PADDRPOOLNODE pAddrNode)
{
	if ((pAddrNodeList == NULL) || (pAddrNode == NULL))
		return -1;

	if (pAddrNodeList->header == NULL)
	{
		pAddrNodeList->header = pAddrNode;
		pAddrNode->next = pAddrNodeList->header;
		pAddrNodeList->curnode = pAddrNode;
	}
	else
	{
		pAddrNodeList->curnode->next = pAddrNode;
		pAddrNode->next = pAddrNodeList->header;
		pAddrNodeList->curnode = pAddrNode;
	}

	pAddrNodeList->length ++;
	return 1;
}

/** 
* @fn    int InitAddrPool(PADDRPOOLLIST pAddrPoolList, PADDR_POOL AddrPool);
 * @brief Initial address pool
 * 
 * @param[out] Address pool list context
 * @param[in]address of pool context
 * @retval 1  success 
 * @retval -1 Failure 
 */
int InitAddrPool(PADDRPOOLLIST pAddrPoolList,
				 PADDR_POOL AddrPool)
{	
	int uAddrIdx = 0;
	PADDRPOOLNODE pNode = NULL;

	pAddrPoolList->header = NULL;
	pAddrPoolList->curnode = NULL;
	pAddrPoolList->length = 0;

	for (uAddrIdx = AddrPool->uiStartIp;
		uAddrIdx <= AddrPool->uiEndIp;
		uAddrIdx ++)
	{
		pNode = AllocAddrPoolNode(uAddrIdx);
		AddAddrPoolNode(pAddrPoolList, pNode);
	}
	
	pAddrPoolList->curnode = pAddrPoolList->header;

	return 1;
}

/** 
* @fn    int AllocAddrPort(PADDRPOOLLIST pList,
*		unsigned int uiRange,
*		unsigned short* usStartPort,
*		unsigned short* usStopPort,
*		unsigned int *Addr);
* @brief From address pool assign address
* 
* @param[in] Port multiple range
* @param[out] Start port
* @param[out] End port
* @param[out] Extern Address 
* @retval  1 success 
* @retval -1 Failure 
*/
int AllocAddrPort(PADDRPOOLLIST pList,
				  unsigned int uiRange,
				  unsigned short* usStartPort,
				  unsigned short* usStopPort,
				  unsigned int *Addr)
{
	PADDRPOOLNODE pBegin = pList->curnode, pCur = NULL;
	int pBit = pList->curnode->validbit;

	pCur = pBegin;
	//If the current address of the port the distribution,Skip to the next address context
	if(pCur->validbit >= 64){
		pCur->validbit = 0;
		pCur = pCur->next;
	}

	if(pCur->validbit + uiRange - 1 >= 64)
		pCur->validbit = pCur->validbit + uiRange;
	else
	{
		//Test port is to take up
		if(portrange_test(pCur->bitmap,
			pCur->validbit,
			pCur->validbit + uiRange - 1) == 0)
			pCur->validbit = pCur->validbit + uiRange; 
		else
		{
			//assign port
			*usStartPort = pCur->validbit * MINI_RANGE_VALUE + MINI_RANGE_VALUE;
			*usStopPort = *usStartPort + MINI_RANGE_VALUE - 1 + (uiRange - 1) * MINI_RANGE_VALUE;

			//set port 
			portrange_set(pCur->bitmap,
				pCur->validbit,
				pCur->validbit + uiRange - 1,
				1);
		
			pCur->validbit += uiRange;
			*Addr = pCur->ip;
	
			//Check whether to port boundary
			if (*usStopPort < *usStartPort)
				goto FALG;

			return 1;
		}
	}
FALG:
	while (1)
	{
		//The distribution
		if((pCur == pBegin) && (pBit == pCur->validbit))
		{
			return -1;
		}
		//If the current address of the port the distribution,Skip to the next address context	
		if (pCur->validbit >= 64){
			pCur->validbit = 0;
			pCur = pCur->next;
			continue;
		}

		if (pCur->validbit + uiRange - 1 >= 64)
		{
			pCur->validbit += uiRange;
			continue;
		}
		//Test port is to take up
		if (portrange_test(pCur->bitmap, pCur->validbit, pCur->validbit + uiRange - 1) != 1)
		{
			pCur->validbit += uiRange;
			continue;
		}
		//set port 
		portrange_set(pCur->bitmap, pCur->validbit, pCur->validbit+uiRange - 1, 1);
		//assign port
		*usStartPort = (pCur->validbit * MINI_RANGE_VALUE + MINI_RANGE_VALUE);
		*usStopPort = *usStartPort + MINI_RANGE_VALUE - 1 + (uiRange - 1) * MINI_RANGE_VALUE;
		
		pCur->validbit += uiRange;
		*Addr = pCur->ip;

		//Check whether to port boundary	
		if (*usStopPort < *usStartPort)
			goto FALG;

		return 1;
	}
	return -1;
}	

/** 
* @fn   int DeallocAddrPort(PADDRPOOLLIST pList,
*		unsigned int uiRange,
*		unsigned short usStartPort,
*		unsigned short usStopPort,
*		unsigned long Addr)
* @brief From address pool release address
* 
* @param[in] Port multiple range
* @param[in] Start port
* @param[in] End port
* @param[in] Extern Address 
* @retval  1 success 
* @retval -1 Failure 
*/
int DeallocAddrPort(PADDRPOOLLIST pList,
				unsigned int uiRange,
				unsigned short usStartPort,
				unsigned short usStopPort,
				unsigned long Addr)
{
	PADDRPOOLNODE pNode = NULL;
	int bit = (usStartPort - MINI_RANGE_VALUE) / MINI_RANGE_VALUE;

	/* validation */
	if ((bit < 0) || (bit >= 64)) 
		return -1;

	if(pList->header->ip == Addr)
	{
		portrange_set(pList->header->bitmap, bit, bit + uiRange - 1, 0);
		return 1;
	}

	pNode = pList->header->next;

	while(pNode != pList->header)
	{
		if(pNode->ip == Addr)
		{
			portrange_set(pNode->bitmap, bit, bit + uiRange - 1, 0);
			return 1;
		}

		pNode = pNode->next;
	}
	return -1;
}

/** 
* @fn   void DestoryAddrPool(PADDRPOOLLIST pList);
* @brief Destory address pool
* 
* @param[in] pList address pool list context
* @retval void 
*/
void DestoryAddrPool(PADDRPOOLLIST pList)
{	
	PADDRPOOLNODE pNode = NULL, pNext = NULL;
	pNode = pList->header->next;

	while (pNode != pList->header)
	{	
		pNext = pNode->next;
		if (pNode)
			free(pNode);

		pNode = pNext;
	}

	free(pNode);
}
