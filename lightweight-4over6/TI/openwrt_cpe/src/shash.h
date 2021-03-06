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


#ifndef _SHASH_H_
#define _SHASH_H_

typedef unsigned int (*SHASH_FUNCTION)(char*, unsigned int);
typedef unsigned int (*KEYCOMPARE_FUNC)(void *, void *, unsigned int);
typedef int (*FRAGCOMPARE_FUNC)(void *, void *);
typedef void (*FRAGNODEDEL_FUNC)(void *);

struct _tag_frag_head
{
	struct _tag_frag_node *front;
	struct _tag_frag_node *next;
};

struct _tag_frag_node
{
	struct _tag_frag_node *front;
	struct _tag_frag_node *next;
	void *pData;
};

struct _tag_bucket_node
{
	struct _tag_bucket_node *next;
	struct _tag_frag_head fraghdr;
	void *pData;
	unsigned int cnt;
};

typedef struct _tag_shashtable
{
	struct _tag_bucket_node bucketnode;
}SHASHTABLE, *PSHASHTABLE;

typedef struct _tag_shashcb
{
	PSHASHTABLE pshashtable;
	unsigned int hashtablesize;
	SHASH_FUNCTION hashfunc;
	KEYCOMPARE_FUNC keycomparefunc;
	FRAGCOMPARE_FUNC fragcomparefunc;
	FRAGNODEDEL_FUNC fragnodedelfunc;
}SHASHCB, *PSHASHCB;

PSHASHCB ShashCreate(unsigned int tablesize,
					 SHASH_FUNCTION hashfunc, 
					 KEYCOMPARE_FUNC keycomparefunc,
					 FRAGCOMPARE_FUNC fragcomparefunc, 
					 FRAGNODEDEL_FUNC fragnodedelfunc);

void ShashDestory(PSHASHCB phashcb);

struct _tag_bucket_node *ShashBucketFind(PSHASHCB phashcb,
	void *pkey, 
	unsigned int keysize);

int ShashFragInsert(PSHASHCB phashcb, 
						struct _tag_frag_head *fraghead,
						void *pdata);

struct _tag_bucket_node *ShashInsert(PSHASHCB phashcb,
									void *pkey,
									unsigned int keysize,
									void *pdata);

int ShashFragDel(PSHASHCB phashcb, 
					struct _tag_frag_head *fraghead);

#endif
