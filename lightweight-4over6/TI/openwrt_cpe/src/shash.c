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


#include "precomp.h"


static void *sHash_MemAlloc(unsigned int len)
{
	return kmalloc(len, GFP_ATOMIC);
}

static void sHash_MemFree(void *buf)
{
	kfree(buf);
	buf = NULL;
}

//create the hash table of fragment
PSHASHCB ShashCreate(unsigned int tablesize,
					 SHASH_FUNCTION hashfunc, 
					 KEYCOMPARE_FUNC keycomparefunc,
					 FRAGCOMPARE_FUNC fragcomparefunc,
					 FRAGNODEDEL_FUNC fragnodedelfunc)
{
	PSHASHCB pshashcb = NULL;

	//allocate the hash control block
	pshashcb = (PSHASHCB)sHash_MemAlloc(sizeof(SHASHCB));
	if (pshashcb == NULL)
		return NULL;

	//hash compute function
	pshashcb->hashfunc = hashfunc;
	//key value compare function
	pshashcb->keycomparefunc = keycomparefunc;
	//fragment packet compare function
	pshashcb->fragcomparefunc = fragcomparefunc;
	//the fragment node timeout function
	pshashcb->fragnodedelfunc = fragnodedelfunc;
	//hash table size
	pshashcb->hashtablesize = tablesize;
	//allocate the hash table space
	pshashcb->pshashtable = (PSHASHTABLE)sHash_MemAlloc(sizeof(SHASHTABLE) * tablesize);
	if (pshashcb->pshashtable == NULL)
	{
		sHash_MemFree(pshashcb);
		return NULL;
	}
	//zero the hash table space
	memset(pshashcb->pshashtable, 0, sizeof(SHASHTABLE) * tablesize);

	return pshashcb;
}


//destory the hash table of fragment
void ShashDestory(PSHASHCB phashcb)
{
	int i = 0;
	struct _tag_bucket_node *pbucketnode = NULL, *pprebucketnode = NULL;
	PFRAG_TUPLE pfragtuple = NULL;

	for (i = 0; i < phashcb->hashtablesize; i ++)
	{
		pprebucketnode = &phashcb->pshashtable[i].bucketnode;
		pbucketnode = phashcb->pshashtable[i].bucketnode.next;
		
		while (pbucketnode != NULL)
		{
			//delete the fragment node from hash table
			if (ShashFragDel(phashcb, &pbucketnode->fraghdr) < 0)
			{
				Log(LOG_LEVEL_NORMAL, "fragment node is null");
			}
			pprebucketnode->next = pbucketnode->next;
			kfree(pbucketnode->pData);
			pbucketnode->pData = NULL;
			kfree(pbucketnode);
			pbucketnode = pprebucketnode->next;
		}
	}	
	//free the hashtable
	if (phashcb->pshashtable != NULL)
		sHash_MemFree(phashcb->pshashtable);

	// free the hash control block
	if (phashcb != NULL)
		sHash_MemFree(phashcb);
}


//find the fragment root node according to the key value
struct _tag_bucket_node *ShashBucketFind(PSHASHCB phashcb,
	void *pkey, 
	unsigned int keysize)
{
	
	struct _tag_bucket_node *pbucket = NULL;
	unsigned int key = 0;

	//compute the hash key value
	key = phashcb->hashfunc((char *)pkey, keysize);
	key %= phashcb->hashtablesize;

	//find the fragment int the bucket
	pbucket = phashcb->pshashtable[key].bucketnode.next;
	while (pbucket != NULL)
	{
		if (phashcb->keycomparefunc(pbucket->pData, pkey, keysize))
		{
			return pbucket;
		}
		pbucket = pbucket->next;
	}

	return NULL;
}

//insert a fragment node a according to the offset in the hash of bucket
int ShashFragInsert(PSHASHCB phashcb, 
					struct _tag_frag_head *fraghead,
					void *pdata)
{
	struct _tag_frag_node *pnode = NULL;
	struct _tag_frag_node *pnewnode = NULL;

	pnewnode = (struct _tag_frag_node *)sHash_MemAlloc(sizeof(struct _tag_frag_node));
	if (pnewnode == NULL)
		return -1;
	memset(pnewnode, 0, sizeof(struct _tag_frag_node));
	
	pnewnode->pData = pdata;
	
	if (fraghead->next == NULL)
	{
		fraghead->next = pnewnode;
		pnewnode->next = NULL;
		pnewnode->front = (struct _tag_frag_node *)fraghead;
	}
	else
	{
		for (pnode = fraghead->next;
				pnode != NULL;
				pnode = pnode->next)
		{
			//compare the offset
			if (phashcb->fragcomparefunc(pnode->pData, pdata) == 0)
			{
				sHash_MemFree(pnewnode);
				break;
			}
			if (phashcb->fragcomparefunc(pnode->pData, pdata) < 0)
			{
				pnode->front->next = pnewnode;
				pnewnode->next = pnode;
				pnewnode->front = pnode->front;
				pnode->front = pnewnode;
				break;
			}
			else
			{
				if (pnode->next == NULL)
				{
					pnode->next = pnewnode;
					pnewnode->front = pnode;
					pnewnode->next = NULL;
					break;
				}
				else
				{
					if (phashcb->fragcomparefunc(pnode->next->pData, pdata) < 0)
					{
						pnode->next->front = pnewnode;
						pnewnode->front = pnode;
						pnewnode->next = pnode->next;
						pnode->next = pnewnode;
						break;
						
					}
					else
					{
						continue;
					}
				}
			}
		}
	}
	
	return 1;
}

//insert a new fragment to the hash tables
struct _tag_bucket_node *ShashInsert(PSHASHCB phashcb,
				void *pkey,
				unsigned int keysize,
				void *pdata)
{
	unsigned int key = 0;
	struct _tag_bucket_node *pnewbucketnode = NULL;

	//compute the hash value
	key = phashcb->hashfunc((char *)pkey, keysize);
	key %= phashcb->hashtablesize;

	//allocate the new bucket node header
	pnewbucketnode = (struct _tag_bucket_node *)sHash_MemAlloc(sizeof(struct _tag_bucket_node));
	if (pnewbucketnode == NULL)
		return NULL;

	memset(pnewbucketnode, 0, sizeof(struct _tag_bucket_node));
	
	pnewbucketnode->pData = pdata;
	//link up the bucket of the end of the node	
	if (phashcb->pshashtable[key].bucketnode.next != NULL)
	{
		pnewbucketnode->next = phashcb->pshashtable[key].bucketnode.next;
		phashcb->pshashtable[key].bucketnode.next = pnewbucketnode;
		return pnewbucketnode;
	}
	
	phashcb->pshashtable[key].bucketnode.next = pnewbucketnode;
	return pnewbucketnode;
}

//delete the fragment node from the hash table
int ShashFragDel(PSHASHCB phashcb, 
					struct _tag_frag_head *fraghead)
{
	struct _tag_frag_node *pnode = NULL, *pbnode = NULL;
	
	pnode = fraghead->next;
	if (pnode == NULL)
	{
		return -1;
	}
	
	do 
	{
		pbnode = pnode->next;
		phashcb->fragnodedelfunc(pnode->pData);
		sHash_MemFree(pnode);
		pnode = pbnode;
	} while (pnode != NULL);

	fraghead->next = NULL;

	return 1;
}
