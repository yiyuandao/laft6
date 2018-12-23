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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/in6.h>
#include <linux/ipv6.h>
#include <linux/icmp.h>
#include <net/ipv6.h>
#include <net/udp.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <net/tcp.h>
#include <linux/string.h>
#include <linux/if_ether.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/rtc.h>

#include <linux/netlink.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <net/net_namespace.h>
#include <net/sock.h>
#include <asm/byteorder.h>
#include "storage.h"
#include "memdebug.h"
#include "bplus.h"
#include "packet.h"
#include "log.h"
/** 
 * @fn   PSTORAGE_CTX InitStorage(void)
 * @brief initialize bptree storage
 * 
 * @param[in] NULL
 * 
 * @retval storage context 
 * @retval NULL Failure 
 */
PSTORAGE_CTX InitStorage(void)
{
	PSTORAGE_CTX pCtx = NULL;
	int idx = 0;

	pCtx = (PSTORAGE_CTX)dbg_alloc_mem(sizeof(STORAGE_CTX));
	if (pCtx == NULL)
		return NULL;
	
	for(idx = 0; idx < ELEMENT_MAX; idx ++)
	{
		//init bptree
		bpinit(&pCtx->BPTreeCtx[idx]);
	}
	
	return pCtx;
}

/** 
 * @fn   int InsertElement(PSTORAGE_CTX pCtx, void *pKey, unsigned int uiLen, void *pElement)
 * @brief Insert a element to the storage
 * 
 * @param[in] pCtx	storage context pointer		
 * @param[in] pKey	insert word of key
 * @param[in] uiLen	insert key of length	
 * @param[in] pElement	insert element
 * 
 * @retval	1 success 
 * @retval -1 Failure 
 */
int InsertElement(PSTORAGE_CTX pCtx,
						PKEY pKey,
						unsigned int uiLen,
						PELEMENT pElement,
						ELEMENT_TYPE eType)
{
	int iRet = 0;

	//insert
	iRet = bpinsert(&pCtx->BPTreeCtx[eType], (char *)pKey, uiLen, pElement);
	if (iRet == -1)
	{
		return -1;
	}

	return 1;
}

/** 
* @fn  void *FindElement(PSTORAGE_CTX pCtx,
							void *pKey,
							unsigned int uiLen,
							ELEMENT_TYPE eType)
 * @brief find a element to storage 
 * 
 * @param[in] pCtx	storage context pointer		
 * @param[in] pKey	word of key
 * @param[in] uiLen key of length	
 * @param[in] eType	find element of type
 * 
 * @retval storage content  success 
 * @retval NULL		Failure 
 */
PELEMENT FindElement(PSTORAGE_CTX pCtx,
				  PKEY pKey,
				  unsigned int uiLen,
				  ELEMENT_TYPE eType)
{
	if (pCtx == NULL)
		return NULL;
	return bpsearch(&pCtx->BPTreeCtx[eType], (char *)pKey, uiLen);
}

/** 
* @fn  void* DeleteElement(PSTORAGE_CTX pCtx,
				void *pKey,
				unsigned int uiLen, 
				ELEMENT_TYPE eType)
 * @brief delete a element from storage 
 * 
 * @param[in] pCtx	storage context pointer		
 * @param[in] pKey	word of key
 * @param[in] uiLen key of length	
 * @param[in] eType	delete element of type
 * 
 * @retval storage content  success 
 * @retval NULL		Failure 
 */
PELEMENT DeleteElement(PSTORAGE_CTX pCtx,
					  PKEY pKey,
					  unsigned int uiLen, 
					  ELEMENT_TYPE eType)
{
	void* pElement = NULL;

	pElement = bpdelete(&pCtx->BPTreeCtx[eType], (char *)pKey, uiLen);

	return pElement;
}

/** 
* @fn  long SizeOfElement(PSTORAGE_CTX pCtx)
 * @brief size of storage
 * 
 * @param[in] pCtx	storage context pointer		
 * @retval size of element in storage
 */
long SizeOfElement(PSTORAGE_CTX pCtx)
{	
	printk("v4:%d,v6:%d\n",
		bptreesize(&pCtx->BPTreeCtx[ELEMENT_4]),
		bptreesize(&pCtx->BPTreeCtx[ELEMENT_6]));
	if (bptreesize(&pCtx->BPTreeCtx[ELEMENT_4]) == bptreesize(&pCtx->BPTreeCtx[ELEMENT_6]))
		return bptreesize(&pCtx->BPTreeCtx[ELEMENT_4]);
	return -1;
}

/** 
* @fn  int ClearStorage(struct BPTree*ptree4,
	struct BPTree*ptree6,
	PELEMENT pElement)
 * @brief Clear bplus node callback function
 * 
 * @param[in] ptree4 bplus tree pointer		
 * @param[in] ptree6 bplus tree pointer		
 * @param[in] pElement	storage context pointer		

 * @retval 1 Success
 * @retval -1 Failure
 */
static int ClearStorage(struct BPTree*ptree4,
	struct BPTree*ptree6,
	PELEMENT pElement)
{
	PTUNNEL_CTX pTunnelCtx = (PTUNNEL_CTX)pElement;
	KEY_V4 key4;
	KEY_V6 key6;
	
	key4.uiExtip = pTunnelCtx->uiExtIP;
	key4.usPort = pTunnelCtx->usStartPort;
	memcpy(key6.ucCltIPv6, pTunnelCtx->ucCltIPv6, 16);
	if (bpdelete(ptree4, (char *)&key4, sizeof(key4)) == NULL)
	{
		Log(LOG_LEVEL_ERROR, "bpdelete 4 error");
	}
	if (bpdelete(ptree6, (char *)&key6, sizeof(key6)) == NULL)
	{
		Log(LOG_LEVEL_ERROR, "bpdelete 6 error");
	}
	
	dbg_free_mem(pTunnelCtx);
	return 1;
}


/** 
* @fn  void DestroyStorage(PSTORAGE_CTX pCtx)
 * @brief Clear bplus node
 * 
 * @param[in] pCtx	storage context pointer		
 * @retval 1 Success
 * @retval -1 Failure
 */
int DestroyStorage(PSTORAGE_CTX pCtx)
{
	return bptraverse(&pCtx->BPTreeCtx[ELEMENT_4],
		&pCtx->BPTreeCtx[ELEMENT_6],
		ClearStorage);
}

/** 
* @fn  void DestructorStorage(PSTORAGE_CTX pCtx)
 * @brief destory storage context
 * 
 * @param[in] pCtx	storage context pointer		
 * @retval void
 */
void DestructorStorage(PSTORAGE_CTX pCtx)
{
	if (pCtx)
	{
		DestroyStorage(pCtx);
		dbg_free_mem(pCtx);
		pCtx = NULL;
	}
}



