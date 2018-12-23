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

void *DBKeyHash_MemAlloc(unsigned int len)
{
	return kmalloc(len, GFP_ATOMIC);
}

void DBKeyHash_MemFree(void *buf)
{
	kfree(buf);
	buf = NULL;
}

//Circulation delete the bucket linked list
static void single_link_loop_delete(PDBKEYHASHTABLE pTable, struct _tag_node_link *head)
{
	struct _tag_node_link *q = NULL, *p = NULL, *s = NULL, *t = NULL;
	unsigned int otherindex = 0;
	
	q = head;
	p = head->next;
	while (p != NULL)
	{		
		if (pTable->DataProc(p->pdata) < 0)
		{
			q->next = p->next;
			if (p->othernode != NULL)
			{
				otherindex = p->otherindex;
				s = &pTable->Bucket[p->otherindex].node_link_head;
				t = pTable->Bucket[p->otherindex].node_link_head.next;

				while (t != NULL)
				{
					if (t == p->othernode)
					{
						s->next = t->next;
						DBKeyHash_MemFree(t);
						t = s->next;
						pTable->DataDel(p->pdata);
						break;
						
					}
					else
					{
						s = t;
						t = t->next;
					}
				}
			}
		 	DBKeyHash_MemFree(p);

			p = q->next;
			if (head == &pTable->Bucket[otherindex].node_link_head)
			{
				q = head;
				p = head->next;
			}
		}
		else
		{
			q = p;
			p = p->next;
		}
	}
}

//insert the node in the bucket link head
static struct _tag_node_link *single_link_head_insert(struct _tag_node_link *head, void *pdata)
{
	struct _tag_node_link *p = NULL, *pNewNode = NULL;
	
	pNewNode = (struct _tag_node_link *)DBKeyHash_MemAlloc(sizeof(struct _tag_node_link));
	if (pNewNode == NULL)
		return NULL;

	memset(pNewNode, 0, sizeof(struct _tag_node_link));
	
	pNewNode->pdata = pdata;
	
	p = head->next;
	head->next = pNewNode;
	pNewNode->next = p;
	
	return pNewNode;
}

//find the node from the bucket link list
static struct _tag_node_link *single_link_find(struct _tag_node_link *head,
											   void *key,
											   DATA_COMPARE dc,
											   unsigned char type)
{
	struct _tag_node_link *p = NULL;
	p = head->next;
	
	while (p != NULL)
	{
		if (dc(key, p->pdata, type) == 1)
		{
			return p;
		}
		p = p->next;
	}

	return NULL;
}

//delete a node from the bucket link list
static void single_link_node_delete(PDBKEYHASHTABLE pTable,
	struct _tag_node_link *head,
	void *key,
	DATA_COMPARE dc,
	unsigned char type)
{
	struct _tag_node_link *p = NULL, *q = NULL, *s = NULL, *t = NULL;

	q = head;
	p = head->next;
	
	while (p != NULL)
	{
		if (dc(key, p->pdata, type) == 1)
		{
			q->next = p->next;
			if (p->othernode != NULL)
			{
				s = &pTable->Bucket[p->otherindex].node_link_head;
				t = pTable->Bucket[p->otherindex].node_link_head.next;
				while (t != NULL)
				{
					if (t == p->othernode)
					{
						s->next = t->next;
						DBKeyHash_MemFree(t);
						t = s->next;
						pTable->DataDel(p->pdata);
						break;
					}
					else
					{
						s = t;
						t = t->next;
					}
				}
			}			
			DBKeyHash_MemFree(p);
			p = q->next;
			break;
		}
		else
		{
			q = p;
			p = p->next;
		}
	}
}

//Circulation delete the bucket linked list
void DBKeyHashTableLoopDelete(PDBKEYHASHTABLE pTable, struct _tag_node_link *head)
{
	single_link_loop_delete(pTable, head);
}

//create the double key of the value hash table	
PDBKEYHASHTABLE DBKeyHashTableCreate(unsigned int BucketCnt,
	HASH_FUNCTION hf,
	DATA_COMPARE dhf, 
	DATA_PROC dpc, 
	DATA_DEL ddel,
	void *lock)
{
	PDBKEYHASHTABLE pHashTable = NULL;

	//allocate the double key of the value hash table context
	pHashTable = (PDBKEYHASHTABLE)DBKeyHash_MemAlloc(sizeof(DBKEYHASHTABLE));

	if (pHashTable == NULL)
		return NULL;
	//hash function
	pHashTable->HashFunc = hf;
	//session compare function
	pHashTable->DataCompFunc = dhf;
	//session process function
	pHashTable->DataProc = dpc;
	//session delete function
	pHashTable->DataDel = ddel;
	//hash table size 
	pHashTable->BucketCnt = BucketCnt;
	//allocate the hash table space
	pHashTable->Bucket = (PSINGLENODE)DBKeyHash_MemAlloc(BucketCnt * sizeof(SINGLENODE));
	if (pHashTable->Bucket == NULL)
	{
		DBKeyHash_MemFree(pHashTable);
		return NULL;
	}
	memset(pHashTable->Bucket, 0, BucketCnt * sizeof(SINGLENODE));
	if (lock != NULL)
	{
		pHashTable->hashlock = lock;
	}
	
	return pHashTable;
}

//destory the double key of the value hash table	
void DBKeyHashTableDestory(PDBKEYHASHTABLE pTable)
{
	unsigned int i = 0;
	
	if (pTable == NULL)
		return ;

	//delete the bucket node in the hash table
	for (i = 0; i < pTable->BucketCnt; i ++)
	{
		single_link_loop_delete(pTable, &pTable->Bucket[i].node_link_head);
	}
	if (pTable->hashlock != NULL)
	{
		kfree(pTable->hashlock);
		pTable->hashlock = NULL;
	}
	DBKeyHash_MemFree(pTable);
}

//insert the value of double key to the hashtable
int DBKeyHashTableInsert(PDBKEYHASHTABLE pTable,
							 void *Key1,
							 unsigned int len1,
							 void *Key2,
							 unsigned int len2,
							 void *pData)
{
	unsigned int  lkey1 = 0, lkey2 = 0; 
	struct _tag_node_link * pNode1 = NULL, *pNode2 = NULL;
	
	if ((pTable == NULL) ||(Key1 == NULL) || (Key2 == NULL) || (pData == NULL))
		return -1;
	
	lkey1 = pTable->HashFunc((char *)Key1, len1);
	lkey1 = lkey1 % pTable->BucketCnt;
	lkey2 = pTable->HashFunc((char *)Key2, len2);
	lkey2 = lkey2 % pTable->BucketCnt;

	pNode1 = single_link_head_insert(&pTable->Bucket[lkey1].node_link_head, pData);
	if (pNode1 == NULL)
		return -1;

	pNode2 = single_link_head_insert(&pTable->Bucket[lkey2].node_link_head, pData);
	if (pNode2 == NULL)
		return -1;

	pNode1->otherindex = lkey2;
	pNode1->othernode = pNode2;
	pNode2->othernode = pNode1;
	pNode2->otherindex = lkey1;

	return 1;
}

//find the node from the hash table
void *DBKeyHashTableFind(PDBKEYHASHTABLE pTable, 
	void *pKey,
	unsigned int len, 
	unsigned char type)
{
	unsigned int lkey = 0;
	struct _tag_node_link *pNode = NULL;
	
	if (pTable == NULL || pKey == NULL)
	{
		return NULL;
	}
	
	lkey = pTable->HashFunc((char *)pKey, len);
	lkey = lkey % pTable->BucketCnt;

	pNode = single_link_find(&pTable->Bucket[lkey].node_link_head,
		pKey,
		pTable->DataCompFunc,
		type);
	if (pNode != NULL)
	{
		return pNode->pdata;
	}
	
	return NULL;
}

//delete the hash table
void DBKeyHashTableDelete(PDBKEYHASHTABLE pTable,
	void *pKey,
	unsigned int len, 
	unsigned type)
	
{
	unsigned int lkey = 0;
	
	if (pTable == NULL || pKey == NULL)
		return ;

	lkey = pTable->HashFunc((char *)pKey, len);
	lkey = lkey % pTable->BucketCnt;

	single_link_node_delete(pTable,
		&pTable->Bucket[lkey].node_link_head,
		pKey,
		pTable->DataCompFunc, 
		type);
}

