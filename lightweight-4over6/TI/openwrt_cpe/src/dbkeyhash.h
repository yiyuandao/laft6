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


#ifndef _DBKEYHASH_H_
#define _DBKEYHASH_H_

typedef unsigned int (*HASH_FUNCTION)(char*, unsigned int );

typedef int (*DATA_COMPARE)(void *, void *, int);

typedef int (*DATA_PROC)(void *);

typedef void (*DATA_DEL)(void *);

struct _tag_node_link
{
	struct _tag_node_link *next;
	struct _tag_node_link *othernode;
	unsigned int otherindex;
	void *pdata;
};

typedef struct _tag_SingleNode
{
	struct _tag_node_link node_link_head;
}SINGLENODE, *PSINGLENODE;

typedef struct _tag_DBKeyHashTable
{
	PSINGLENODE	Bucket;
	unsigned int 	BucketCnt;
	HASH_FUNCTION HashFunc;
	DATA_COMPARE DataCompFunc;
	DATA_PROC DataProc;
	DATA_DEL DataDel;
	void *hashlock;
}DBKEYHASHTABLE, *PDBKEYHASHTABLE;

void DBKeyHashTableLoopDelete(PDBKEYHASHTABLE pTable, struct _tag_node_link *head);

PDBKEYHASHTABLE DBKeyHashTableCreate(unsigned int BucketCnt,
	HASH_FUNCTION hf,
	DATA_COMPARE dhf, 
	DATA_PROC dpc, 
	DATA_DEL ddel,
	void *lock);

void DBKeyHashTableDestory(PDBKEYHASHTABLE pTable);

int DBKeyHashTableInsert(PDBKEYHASHTABLE pTable,
							void *Key1,
							unsigned int len1,
							void *Key2,
							unsigned int len2,
							void *pData);

void *DBKeyHashTableFind(PDBKEYHASHTABLE pTable, 
	void *pKey,
	unsigned int len, 
	unsigned char type);

void DBKeyHashTableDelete(PDBKEYHASHTABLE pTable,
	void *pKey,
	unsigned int len, 
	unsigned type);


void *DBKeyHash_MemAlloc(unsigned int len);

void DBKeyHash_MemFree(void *buf);

#endif
