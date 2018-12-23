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
#include "bplus.h"
#include "memdebug.h"
#include "global.h"


struct BPInternalLinkNode* bpInternalLinkNode_alloc(char*,short);
void bpInternalLinkNode_free(struct BPInternalLinkNode*);
struct BPNode* bpNode_alloc(char*,short);
void bpNode_free(struct BPNode*);
struct BPValue* bpValue_alloc(char*,short);
void bpValue_free(struct BPValue*);




/* alloc function for bplus key */
struct BPKey* bpgeneratekey(char* buf,short len)
{
   struct BPKey* pKey = (struct BPKey*)dbg_alloc_mem(sizeof(struct BPKey));
   if(!pKey) return NULL;
  
   pKey->addr = (char*)dbg_alloc_mem(len);
  
   if(!(pKey->addr))
   {
     dbg_free_mem(pKey);
     return NULL;
   } 
   memcpy(pKey->addr,buf,len);
   pKey->length = len;
   return pKey;
}

void bpfreekey(struct BPKey* pKey)
{
   if(!pKey) return;
   dbg_free_mem(pKey->addr);
   dbg_free_mem(pKey);
}

/*alloc function for BPInternalLinkNode type object */

int bpinit(struct BPTree* ptree)
{
	if (ptree == NULL) return -1;
	ptree->root = NULL;
	ptree->header = NULL;
	ptree->size =0;
	return 0;
}

int bptreesize(struct BPTree* ptree)
{
	return ptree->size;
}

int bpcmp(struct BPKey* key1,struct BPKey* key2)
{
	return  memcmp(key1->addr,key2->addr,key1->length);
}

int bpheight(struct BPTree* ptree)
{
	int i =0;
  	struct BPNode* curNode = NULL;
  	if(ptree == NULL) return -1;
  	if((ptree->root == NULL) ||(ptree->header == NULL)) return 0;
  
  	curNode = ptree->root;
  	while(1)
  	{
		i++;
		if(curNode->interLinkHeader->type == 1)
        {
           break;
       	}
        else
        {
           curNode = curNode->interLinkHeader->content.pNode;
        }
    }
    return i;
}

/*void bpprintchar(char c)
{
    int a = (c&0xF0)>>4;
    int b = c&0xF;
   
    if(a<10)  printf("%d",a);
    else
   	{
   		 switch((int)a)
         {
    	 	case 10:
		  		printf("%c",'A');
				break;
	  		case 11:
		  		printf("%c",'B');
				break;
			case 12:
		  		printf("%c",'C');
				break;
			case 13:
		  		printf("%c",'D');
				break;
			case 14:
		  		printf("%c",'E');
				break;
			case 15:
		  		printf("%c",'F');
				break;
			default :
				break;
    	}
   	}

	if(b<10)  printf("%d",b);
    else
   	{
   		 switch((int)b)
         {
    		case 10:
		  		printf("%c",'A');
				break;
	  		case 11:
		  		printf("%c",'B');
				break;
			case 12:
		  		printf("%c",'C');
				break;
			case 13:
		  		printf("%c",'D');
				break;
			case 14:
		  		printf("%c",'E');
				break;
			case 15:
		  		printf("%c",'F');
				break;
			default :
				break;
    	}
   	}
    return;
}
*/
int bptraverse(struct BPTree *up, struct BPTree *down, PFUNC pfunc)
{
   struct BPValue* curValue = NULL,*next;

   /* validation */
   if(up == NULL)
	   return -1;

   if((up->root == NULL)||(up->header == NULL))
	   return 0;

   curValue = up->header;

   while(curValue != NULL)
   {
	
	next = curValue->next;
	spin_lock(&GlobalCtx.StorageLock);
	pfunc(up, down, curValue->pRecord);
	spin_unlock(&GlobalCtx.StorageLock);
  	curValue = next;
   }
 
   return 0;
}

// split the node
int bpsplit(struct BPTree* ptree,struct BPNode* pnode)
{
     struct BPInternalLinkNode* pInternal = NULL,*curInternal=NULL;
     int i = 0;
     struct BPNode* curNode = NULL,*parentNode=NULL;

    /* validation */
    if(ptree == NULL) return -1;
    if((ptree->root == NULL)||(ptree->header == NULL)||(pnode ==NULL))
     {
       return -1;
     }

    if((pnode->parent == NULL)&& (ptree->root != pnode))
     {
       return -1;
     }
	  
    if(pnode->pointerNum <= MAXDEGREE)
     {
       return 0;
     }   

     /* get the midle internal link node */
     for(i=1,pInternal= pnode->interLinkHeader;i<=MINDEGREE;i++)
      {
        pInternal = pInternal->next;
      }
     
     /* create a new bpnode */
	 curNode = bpNode_alloc(pnode->key->addr,pnode->key->length);
	 if(curNode == NULL) return -1;
	 
     curNode ->pointerNum = pnode->pointerNum - MINDEGREE -1;
     curNode ->interLinkHeader = pInternal->next;
     curNode->interLinkHeader->prev = pnode->interLinkHeader->prev;
     curNode->interLinkHeader->prev->next =curNode->interLinkHeader;

     /* update the bpnode to be splited */
     
     memcpy(pnode->key->addr,pInternal->key->addr,pInternal->key->length);
     pnode->key->length = pInternal->key->length;
     pnode ->pointerNum = MINDEGREE +1;
     pInternal->next = pnode->interLinkHeader;
     pnode->interLinkHeader->prev = pInternal;
     
      parentNode = pnode->parent;

	  /* create a new root for the b+ tree */
      if(parentNode == NULL)
      {
		  parentNode = bpNode_alloc(curNode->key->addr,curNode->key->length);
		  if(parentNode == NULL)
		  {
			  bpNode_free(curNode);
			  return -1;
		  }
	      parentNode->parent = NULL;
	      parentNode ->pointerNum = 2;
	         
		  pInternal = bpInternalLinkNode_alloc(pnode->key->addr,pnode->key->length);
		  if(pInternal == NULL)
		  {
			  bpNode_free(curNode);
			  bpNode_free(parentNode);
			  return -1;
		  }

		  curInternal = bpInternalLinkNode_alloc(curNode->key->addr,curNode->key->length);
		  if(curInternal == NULL)
		  {
			  bpNode_free(curNode);
			  bpNode_free(parentNode);
			  bpInternalLinkNode_free(pInternal);
			  return -1;
		  }
	          parentNode->interLinkHeader = pInternal;
			  pInternal->type = 0;
			 
			  pInternal->next = curInternal;
	                  pInternal->prev = curInternal;
	                  pInternal->content.pNode = pnode;

			  
			  curInternal->type =0;
			  curInternal->prev = pInternal;
			  curInternal->next = pInternal;
	          curInternal->content.pNode = curNode;        
	          
	            pnode->parent =  parentNode;        
	            curNode->parent = parentNode; 
	          ptree->root = parentNode;
	        /* adjust curNode's children's parent pointers */
	        if(curNode->interLinkHeader->type== 0)
	        {
	           (curNode->interLinkHeader->content.pNode)->parent = curNode;
	           pInternal = curNode->interLinkHeader->next;
	           while(pInternal != curNode->interLinkHeader)
				 {
					(pInternal->content.pNode)->parent = curNode;
					pInternal = pInternal ->next;
				 }
	        }
			return 0;
      }
      
	 curInternal = bpInternalLinkNode_alloc(pnode->key->addr,pnode->key->length);
	 if(curInternal == NULL)
	 {
		 bpNode_free(curNode);
		 return -1;
	 }
	
      curInternal->type =0;
      

      pInternal = parentNode->interLinkHeader;
      while(bpcmp(pInternal->key,curInternal->key)<0)
       {
           pInternal = pInternal->next;
       }

       if(pInternal == parentNode->interLinkHeader)
        {
           parentNode->interLinkHeader = curInternal;
        }

        curInternal->next = pInternal->prev->next;
        curInternal->prev = pInternal->prev;
        pInternal->prev->next = curInternal;
        pInternal->prev = curInternal;

        parentNode->pointerNum ++;
        curInternal->next->content.pNode = curNode;
        (curInternal->content.pNode)=pnode;
        curNode->parent = parentNode;

        /* adjust curNode's children's parent pointers */
        if(curNode->interLinkHeader->type== 0)
        {
           (curNode->interLinkHeader->content.pNode)->parent = curNode;
           pInternal = curNode->interLinkHeader->next;
           while(pInternal != curNode->interLinkHeader)
             {
                (pInternal->content.pNode)->parent = curNode;
                pInternal = pInternal ->next;
             }
        }
        
      bpsplit(ptree,parentNode);
      return 0;
}

int bpmerge(struct BPTree* ptree, struct BPNode* pnode)
{
     struct BPNode* broNode = NULL;
     struct BPInternalLinkNode* pInternal = NULL,*curInternal = NULL;
     struct BPInternalLinkNode* left=NULL,*right = NULL,*p=NULL; 

     /* validation */
     if((ptree == NULL)||(pnode==NULL))
	 	return -1;
     if((ptree->root == NULL)||(ptree->header == NULL))
        return 0;
     if(pnode->pointerNum >= MINDEGREE)
        return 0;
     if((pnode->parent== NULL)&&(ptree->root == pnode))
        return -1;

     /* locate pnode's brother nodes */
     broNode = pnode->parent;
     pInternal = broNode->interLinkHeader;
     while(pInternal->content.pNode !=pnode)
       {
          pInternal = pInternal ->next;
       }
     if(pInternal == broNode->interLinkHeader)
       {
          left = NULL;
          right = pInternal->next;
       }
     else if(pInternal == broNode->interLinkHeader->prev)
       {
          left = pInternal->prev;
          right = NULL;
       }
     else 
       {
          left = pInternal->prev;
          right = pInternal ->next;
       } 

     /* merge with the left brother node */
     if(left != NULL)
       {
          /* adjust left's subnodes 's parent field */
          p = left ->content.pNode->interLinkHeader;
          if(p ->type ==0)
           {
             p->content.pNode->parent = pnode;
             p=p->next;
           }
          while(p != left->content.pNode->interLinkHeader)
           {
              if(p->type == 0)
                p->content.pNode->parent = pnode;
              p = p->next;
           }

          broNode = left->content.pNode;
          pInternal = broNode->interLinkHeader;
          curInternal = pnode->interLinkHeader->prev;
          curInternal->next = pInternal;
         
          pInternal ->prev->next = pnode->interLinkHeader;
          pnode->interLinkHeader->prev = pInternal->prev;
          pInternal->prev = curInternal;

           pnode ->pointerNum += broNode->pointerNum;
           pnode->interLinkHeader = broNode->interLinkHeader;

          if(left == pnode->parent->interLinkHeader)
           {
             left->next->prev = left->prev;
             left->prev->next = left->next;
             pnode->parent->interLinkHeader = left->next;
           }
           else
            {
              left->next->prev = left->prev;
              left->prev->next = left->next;
            }
            dbg_free_mem(left->key->addr);dbg_free_mem(left->key);dbg_free_mem(left);
            pnode->parent->pointerNum --;
            dbg_free_mem(broNode->key->addr);dbg_free_mem(broNode->key);dbg_free_mem(broNode);

            /* if the pnode needs to split */
            if((pnode->parent == ptree ->root)&&(ptree->root->pointerNum ==1))
              {
                 broNode = ptree ->root;
                 ptree ->root = pnode;
                 pnode->parent = NULL;
                 bpInternalLinkNode_free(broNode->interLinkHeader);
                 bpNode_free(broNode);
              }

            if(pnode->pointerNum > MAXDEGREE)
             {
               bpsplit(ptree,pnode);
             }
            else
             {
		    	bpmerge(ptree,pnode->parent);  
             } 
            return 0;
           
       }
     else if(right != NULL)
      {
          /* adjust pnode 's parent field */
          p = pnode ->interLinkHeader;
          if(p ->type ==0)
           {
              p->content.pNode->parent = right->content.pNode;
              p = p->next;
           }
          while(p!= pnode->interLinkHeader)
           {
              if(p ->type ==0)
                 p ->content.pNode->parent = right->content.pNode;
               p = p->next;
           }

          broNode = right->content.pNode;
          pInternal = broNode -> interLinkHeader;
          curInternal = broNode->interLinkHeader->prev;
          pInternal->prev->next = pnode->interLinkHeader;
          pInternal->prev = pnode->interLinkHeader->prev;
          pnode->interLinkHeader->prev->next =pInternal;
          pnode->interLinkHeader->prev = curInternal;

          broNode ->interLinkHeader = pnode->interLinkHeader;

           broNode ->pointerNum += pnode->pointerNum;
           curInternal = right->prev;
          if(curInternal == pnode->parent->interLinkHeader)
           {
             right->prev = curInternal->prev;
             curInternal->prev->next = right;
             pnode->parent->interLinkHeader = right;
           }
           else
            {
              right->prev = curInternal->prev;
              curInternal->prev->next = right;
            }
            dbg_free_mem(curInternal->key->addr);dbg_free_mem(curInternal->key);dbg_free_mem(curInternal);
            pnode->parent->pointerNum --;
            dbg_free_mem(pnode->key->addr);dbg_free_mem(pnode->key);dbg_free_mem(pnode);

            /* if the pnode needs to split */
            if((broNode->parent == ptree->root)&&(ptree->root->pointerNum ==1))
			{
		    	pnode = ptree->root;
		    	ptree->root = broNode;
                broNode ->parent = NULL;
		     	bpInternalLinkNode_free(pnode ->interLinkHeader);
			    bpNode_free(pnode);
            }

            if(broNode->pointerNum > MAXDEGREE)
             {
               bpsplit(ptree,broNode);
             }
            else
             {
        		 bpmerge(ptree,broNode->parent);
             }
             return 0;
      }      
}


/* insert a node */
/* failed, return -1; succeed, return 0; */
int _bpinsert(struct BPTree* ptree,struct BPValue* curValue)
{
   struct BPInternalLinkNode* pInternal=NULL,*curInternal=NULL;
   struct BPNode* curNode=NULL;
   
   /*validation*/
   if((ptree==NULL)||(curValue == NULL))
   	return -1;
   if(ptree->size == MAXBPSIZE) return -1;
  
   /* when the b+ tree is empty */
   if(ptree->root == NULL)
    {
	  pInternal = bpInternalLinkNode_alloc(curValue->key->addr,curValue->key->length);
	  if(pInternal == NULL)
	  {
	  	//printf("bpInternalLinkNode_alloc return NULL\n");
	  	return -1;
	  }
	  curNode = bpNode_alloc(curValue->key->addr,curValue->key->length);
	  if(curNode == NULL)
	  {
	  	//printf("bpNode_alloc return NULL\n");

		  bpInternalLinkNode_free(pInternal);
		  return -1;
	  }
      pInternal->next=pInternal;
      pInternal->prev = pInternal;
      pInternal->type= 1;
      pInternal->content.pValue = curValue;
      
      curNode->interLinkHeader = pInternal;
      curNode->pointerNum = 1;
      curNode->parent = NULL;
      
      curValue->prev = curValue ->next =NULL;
      ptree->header = curValue;
      ptree->root = curNode;
      ptree->size ++;
      return 0;
    }

    curNode = ptree->root;

    /* when the new key is bigger than root's key */
    if(bpcmp(curValue->key, curNode->key)>0)
	{
		// when the root is a leaf
		if(curNode->interLinkHeader->type ==1)
		{
            pInternal = bpInternalLinkNode_alloc(curValue->key ->addr,curValue->key->length);
			if(pInternal == NULL) return -1;
			pInternal->content.pValue = curValue;
			pInternal->type =1;
                       
			pInternal->next= curNode->interLinkHeader;
			pInternal->prev= curNode->interLinkHeader->prev;
			curNode->interLinkHeader->prev->next = pInternal;
			curNode->interLinkHeader->prev = pInternal;
                        
            /* construct doublely linked list */
			curValue->prev = pInternal->prev->content.pValue;
            curValue->next = NULL;
            ( pInternal->prev->content.pValue)->next = curValue;
 
			curNode->pointerNum ++;
			//curNode->key = curValue->key;
			memcpy(curNode->key->addr,curValue->key->addr,curValue->key->length);
			bpsplit(ptree,curNode);
			ptree->size ++;
			return 0;
		}
                
        /* when the root is not a leaf */
        pInternal = bpInternalLinkNode_alloc(curValue ->key->addr,curValue ->key ->length);
        if(!pInternal) 
			return -1;
		while(curNode->interLinkHeader->type ==0)
		{
			memcpy(curNode->key->addr,curValue->key->addr,curValue->key->length);
			//curNode->key->length = curValue->key->length;
            memcpy(curNode ->interLinkHeader->prev->key->addr,curValue->key->addr,curValue->key->length);
			curNode = curNode->interLinkHeader->prev->content.pNode;
		}
		
	    pInternal->content.pValue = curValue;
		pInternal->type =1;

		pInternal->next= curNode->interLinkHeader;
		pInternal->prev= curNode->interLinkHeader->prev;
		curNode->interLinkHeader->prev->next = pInternal;
		curNode->interLinkHeader->prev = pInternal;

        /* construct doublely linked list */
		curValue->prev = pInternal->prev->content.pValue;
        curValue->next = NULL;
        (pInternal->prev->content.pValue)->next = pInternal->content.pValue;
             
		curNode->pointerNum ++;
		memcpy(curNode->key->addr,curValue->key->addr,curValue->key->length);
		//curNode->key->length = curValue->key->length;
		bpsplit(ptree,curNode);
		ptree->size ++;
		return 0;
	}
    
	/* other cases */
    while(curNode->interLinkHeader->type == 0)
     {
         pInternal = curNode->interLinkHeader;
	     while(bpcmp(pInternal->key,curValue->key)<0)
		         pInternal = pInternal->next;
	     if(bpcmp(pInternal->key , curValue->key)==0)
			 return 0;
	     else
	      {
        	curNode=pInternal->content.pNode;
	      }
     }

	pInternal = curNode->interLinkHeader;
    while(bpcmp(pInternal->key,curValue->key) <0)
	   pInternal = pInternal->next;
	if(bpcmp(pInternal->key, curValue->key) ==0)
			 return 0;
	else
		 {
			 
              curInternal = bpInternalLinkNode_alloc(curValue->key->addr,curValue->key->length);
              if(!curInternal) return -1;
			   
              if(pInternal == curNode->interLinkHeader)
              {
                 curNode->interLinkHeader = curInternal;
              }
			  
			 curInternal ->content.pValue = curValue;
			 curInternal->type =1;
                         
			 curInternal->next= pInternal;
			 curInternal->prev= pInternal->prev;
			 pInternal->prev->next = curInternal;
             pInternal->prev = curInternal;
			
                         /* construct doublely linked list */
			 if(bpcmp(curInternal->prev->key,curValue->key)>0)
			 {
                   curValue->prev = (pInternal->content.pValue)->prev;
                   if(curValue->prev != NULL)
                      {
                         ( pInternal->content.pValue)->prev->next = curValue;
                      }
                      else
                      {
                          ptree->header = curValue;
                      }
                      curValue->next = pInternal->content.pValue;
                      (pInternal->content.pValue)->prev = curValue;
             }
             else
             {
                      pInternal = curInternal->prev;
                      (pInternal->content.pValue)->next->prev = curValue;
                       curValue->next = (pInternal->content.pValue)->next;
                      (pInternal->content.pValue)->next = curValue;
                      curValue->prev = pInternal->content.pValue;
              }
			curNode->pointerNum ++;
			bpsplit(ptree,curNode);
			ptree->size ++;
			return 0;
		 }

}

int bpinsert(struct BPTree *tree,char *addr,short len,void *record)
{
	struct BPValue *pvalue = NULL;
	int ret = 0;
	
	if(!tree)
		 return -1;
	pvalue = bpValue_alloc(addr,len);
	if(!pvalue)
		return -1;

	pvalue->pRecord = record;
	ret = _bpinsert(tree,pvalue);
	
	//printf("ret : %d\n", ret);
	return ret;
}

/* delete a node 
* failed, return NULL;
*/
struct BPValue* _bpdelete(struct BPTree* ptree,struct BPKey key)
{
   struct BPInternalLinkNode* pInternal=NULL;
   struct BPNode* curNode=NULL,*delNode=NULL;
   struct BPKey newKey;
	struct BPValue *ret = NULL;

   /* validation */
   if(ptree == NULL) return NULL;
  
   /* when the b+ tree is empty */
   if((ptree->root == NULL)||(ptree->header == NULL))
    {
       return NULL;
    }

   /* when the key is bigger than the root's key */
   if(bpcmp(&key,ptree->root->key)>0) return NULL;

   /* search the node to be deleted */
   delNode = ptree->root;
   while(delNode->interLinkHeader->type == 0)
   {
	   pInternal = delNode->interLinkHeader;
	   while(bpcmp(pInternal->key,&key)<0)
	   {
		   pInternal = pInternal->next;
	   }
	   delNode=pInternal->content.pNode;		 
   }

   /* delete the last node of b+ tree */
   if((delNode == ptree->root) && (delNode->pointerNum ==1))
   	{
   	    if(bpcmp(delNode->key,&key) !=0)
         {
			return NULL;
         }
	    pInternal = delNode->interLinkHeader;
      	ret = pInternal->content.pValue; 
		bpInternalLinkNode_free(pInternal);
		
		bpNode_free(delNode);
	   ptree->root = NULL;
	   ptree->header = NULL;
	   ptree->size =0;
	   return ret;

   	}

   /* adjust the tree when the key to be deleted equals to a node's key */
   if(bpcmp(delNode->key,&key)==0)
   {
	 curNode = delNode;
	 pInternal = curNode->interLinkHeader->prev;
	 curNode->interLinkHeader->prev = pInternal->prev;
	 pInternal->prev->next = curNode->interLinkHeader;
	 curNode->pointerNum --;
	 
	 memcpy(curNode->key->addr,curNode->interLinkHeader->prev->key->addr,curNode->key->length);
	

        /* adjust bpvalue linked list */
        if((pInternal->content.pValue)->prev == NULL)
         {
             ptree->header = (pInternal->content.pValue)->next;
             (pInternal->content.pValue)->next->prev = NULL;
         }
         else
         {
             (pInternal->content.pValue)->prev->next = (pInternal->content.pValue)->next;
             if((pInternal->content.pValue)->next !=NULL)
              {
                 (pInternal->content.pValue)->next->prev = (pInternal->content.pValue)->prev;
              }
         }
		 ret = pInternal->content.pValue;
		 bpInternalLinkNode_free(pInternal);

	   if(curNode->parent ==NULL)
        {
          ptree ->size --; 
	   	  return ret;
	    } 
	   newKey.addr = (char*)dbg_alloc_mem(curNode->key->length);
	   newKey.length = curNode->key->length;
	   memcpy(newKey.addr,curNode->key->addr,curNode->key->length);
	   
       curNode = curNode->parent;
	   while(curNode != NULL)
	   {
           if(bpcmp(curNode->key,&key) ==0)
            {		   
		      memcpy(curNode->key->addr,newKey.addr,newKey.length);
		      curNode->key->length = newKey.length;
		      memcpy(curNode->interLinkHeader->prev->key->addr,newKey.addr,newKey.length);
		      curNode->interLinkHeader->prev->key->length = newKey.length;
		      curNode = curNode->parent;
            }
           else
            {
              pInternal = curNode ->interLinkHeader;
              while(bpcmp(pInternal->key,&key) !=0)
              {
                pInternal =pInternal->next;
              }
              memcpy(pInternal->key->addr,newKey.addr,newKey.length);
              break;
            }
	   }

       dbg_free_mem(newKey.addr);
	   bpmerge(ptree,delNode);
	   ptree->size --;
	   return ret;
   }

   /* other cases */
   pInternal = delNode->interLinkHeader;
   while(bpcmp(pInternal->key,&key) <0)
	   pInternal = pInternal->next;
   if(bpcmp(pInternal->key,&key) ==0)
   {
        if(pInternal == delNode->interLinkHeader)
         {
             delNode->interLinkHeader = pInternal->next;
         }
	    pInternal->prev->next = pInternal->next;
	    pInternal->next->prev = pInternal->prev;
 
        /* adjust bpvalue linked list */
        if((pInternal->content.pValue)->prev == NULL)
         {
             ptree->header = (pInternal->content.pValue)->next;
             (pInternal->content.pValue)->next->prev = NULL;
         }
         else
         {
             (pInternal->content.pValue)->prev->next = (pInternal->content.pValue)->next;
             if((pInternal->content.pValue)->next !=NULL)
              {
                ( pInternal->content.pValue)->next->prev = (pInternal->content.pValue)->prev;
              }
         }
       ret = pInternal->content.pValue; 
	   bpInternalLinkNode_free(pInternal);
	   delNode->pointerNum --;
	   bpmerge(ptree,delNode);
	   ptree->size --;
	   return ret;
   }
   else
    {
	  return NULL;  
    } 

}

void* bpdelete(struct BPTree *tree,char *addr,short len)
{
	struct BPValue *pvalue = NULL;
	struct BPKey *pkey = NULL;
	void *record = NULL;

	if((!tree)||(!addr))
		return NULL;
	pkey = bpgeneratekey(addr,len);
	if(!pkey)
		return NULL;

	pvalue = _bpdelete(tree,*pkey);
	bpfreekey(pkey);

	if(pvalue)
	{
		record = pvalue->pRecord;
		bpValue_free(pvalue);
	}
	return record;		
}

/* failed ,return NULL;*/
struct BPValue* _bpsearch(struct BPTree* ptree,struct BPKey key)
{
	struct BPInternalLinkNode* pInternal=NULL;
	struct BPNode* curNode=NULL;

	if(ptree == NULL) return NULL;
	/* when the b+ tree is empty */
	if((ptree->root == NULL) ||(ptree ->header == NULL)) return NULL;
	
	/* when the key is bigger than the root's key */
	if(bpcmp(&key,ptree->root->key)>0) return NULL;

	curNode = ptree->root;
	while(curNode->interLinkHeader->type == 0)
    {
    	pInternal = curNode->interLinkHeader;
		while(bpcmp(pInternal->key,&key)<0)
		{
			 pInternal = pInternal->next;
		}		 
		curNode=pInternal->content.pNode;		 
   	}
	pInternal = curNode->interLinkHeader;
    while(bpcmp(pInternal->key,&key)<0)
	      pInternal = pInternal->next;
	if(bpcmp(pInternal->key ,&key) ==0)
		return pInternal->content.pValue;
	else
		return NULL;
}


void* bpsearch(struct BPTree *tree,char *addr,short len)
{
	struct BPValue *pvalue = NULL;
	struct BPKey *pkey = NULL;
	void *record = NULL;

	if((!tree)||(!addr))
		return NULL;
	pkey = bpgeneratekey(addr,len);
	if(!pkey)
		return NULL;

	pvalue = _bpsearch(tree,*pkey);
	bpfreekey(pkey);

	if(pvalue)
	{
		record = pvalue->pRecord;
		//bpValue_free(pvalue);
	}
	return record;		

}

/*alloc function for BPInternalLinkNode type object */
struct BPInternalLinkNode* bpInternalLinkNode_alloc(char* keyAddr,short keyLen)
{
	struct BPKey *pKey = NULL;
	struct BPInternalLinkNode* pNode = NULL;
	if((pKey = bpgeneratekey(keyAddr,keyLen)) == NULL) return NULL;
	

	if((pNode = (struct BPInternalLinkNode*)dbg_alloc_mem(sizeof(struct BPInternalLinkNode))) == NULL)
	{
		bpfreekey(pKey);
		return NULL;
	}
	pNode->key = pKey;
	pNode->next = pNode->prev = NULL;
	pNode->type =0;
	return pNode;

}

/* dealloc function for BPInternalLinkNode type object */
void bpInternalLinkNode_free(struct BPInternalLinkNode* pNode)
{
	if(!pNode) return;
	bpfreekey(pNode->key);
	dbg_free_mem(pNode);
}

struct BPNode* bpNode_alloc(char* keyAddr,short keyLen)
{
	struct BPKey* pKey = NULL;
	struct BPNode* pNode = NULL;
	if((pKey = bpgeneratekey(keyAddr,keyLen)) == NULL) return NULL;

	if((pNode = (struct BPNode*)dbg_alloc_mem(sizeof(struct BPNode))) == NULL)
	{ 
		bpfreekey(pKey);
		return NULL;
	}
	pNode->key = pKey;
	pNode->parent = NULL;
	pNode->pointerNum =0;
	pNode->interLinkHeader = NULL;

	return pNode; 
}

void bpNode_free(struct BPNode* pNode)
{
	if(!pNode) return;
	bpfreekey(pNode ->key);
	dbg_free_mem(pNode);
}

struct BPValue* bpValue_alloc(char* keyAddr,short keyLen)
{
	struct BPKey* pKey = NULL;
	struct BPValue* pValue = NULL;
	if((pKey = bpgeneratekey(keyAddr,keyLen)) ==NULL) return NULL;

	if((pValue = (struct BPValue*)dbg_alloc_mem(sizeof(struct BPValue))) == NULL)
	{
		bpfreekey(pKey);
		return NULL;
	}
	pValue->key = pKey;
	pValue->next = NULL;
	pValue->prev = NULL;
	pValue->pRecord = NULL;
	return pValue;
}

void bpValue_free(struct BPValue* pValue)
{
    if(!pValue) return ;
	bpfreekey(pValue->key);
	dbg_free_mem(pValue);
}

