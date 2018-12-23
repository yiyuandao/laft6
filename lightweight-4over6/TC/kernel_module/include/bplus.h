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
#ifndef _BPLUS_H_
#define _BPLUS_H_

#define MINDEGREE  4 
#define MAXDEGREE  8 
#define MAXBPSIZE  600000


struct BPNode;
struct BPValue;
struct BPKey;

struct BPKey
{
   char* addr;
   short length;
};

union BPContent
{
   struct BPNode* pNode;
   struct BPValue* pValue;
};
   

struct BPInternalLinkNode
{
   union BPContent content;
   struct BPKey* key;
   struct BPInternalLinkNode* next;
   struct BPInternalLinkNode* prev;
   short type; // 0 internal node;1 value
};
// bplus tree node
struct BPNode
{
  struct BPKey* key;
  struct BPNode* parent;
  short pointerNum;
  struct BPInternalLinkNode* interLinkHeader; 
};

// bplus tree value
// doublely linked list,not cyclic
struct BPValue
{
  /* the key of the record */
  struct BPKey* key;
  /* two pointers to construct a linked list */
  struct BPValue* next;
  struct BPValue* prev;
  /* point to the record */
  void* pRecord;
};

// bptree
struct BPTree
{
  struct BPNode* root;
  struct BPValue* header;
  int size;
};

typedef int (*PFUNC)(struct BPTree*,struct BPTree*,void*);
/* when succeed, return 0 */
int bpinit(struct BPTree*);
int bptreesize(struct BPTree* ptree);
int bpinsert(struct BPTree* tree,char *addr,short len, void *record);
void* bpdelete(struct BPTree* tree,char *addr,short len );
void* bpsearch(struct BPTree* tree,char *addr,short len);
int bpheight(struct BPTree*);
int bptraverse(struct BPTree*,struct BPTree*, PFUNC);

#endif
