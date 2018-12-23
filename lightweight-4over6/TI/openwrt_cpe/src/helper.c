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
#include <linux/spinlock_types.h>

//AP hash algorithm
unsigned int APHash(char* str, unsigned int len)
{
	unsigned int hash = 0;
	unsigned int i    = 0;

	for(i = 0; i < len; str++, i++)
	{
		hash ^= ((i & 1) == 0) ? (  (hash <<  7) ^ (*str) ^ (hash >> 3)) :
			(~((hash << 11) ^ (*str) ^ (hash >> 5)));
	}

	return (hash & 0x7FFFFFFF);
}

//checksum algorithm
unsigned short ip_checksum(unsigned short* buffer, int size) 
{
	unsigned long cksum = 0;

	// Sum all the words together, adding the final byte if size is odd

	while (size > 1) 
	{
		cksum += *buffer++;
		size -= sizeof(unsigned short);
	}
	if (size)
	{
		cksum += *(unsigned char*)buffer;
	}

	// Do a little shuffling

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);

	// Return the bitwise complement of the resulting mishmash

	return (unsigned short)(~cksum);
}


void streamprint(unsigned char *buf, unsigned int len)
{
	int i = 0;
	
	for (i = 0; i < len; i ++)
	{
		printk("%02x ", buf[i]);
	}
	printk("\n");
}

/*implemented according with RFC 1624, modified the algorithm from RFC 1071 and 1141*/
unsigned short csum_incremental_update(unsigned short old_csum,

                unsigned short old_field,

                unsigned short new_field)

{

    unsigned long csum = (~old_csum & 0xFFFF) + (~old_field & 0xFFFF) + new_field ;

    csum = (csum >> 16) + (csum & 0xFFFF);

    csum +=  (csum >> 16);

    return ~csum;

}

void print_tuple(PSESSIONTUPLES psessiontuples)
{
	printk("srcip:0x%x\n", psessiontuples->srcip);
	printk("dstip:0x%x\n", psessiontuples->dstip);
	printk("olodport:0x%x\n", psessiontuples->oldport);
	printk("newport:0x%x\n", psessiontuples->newport);
	printk("proto:0x%x\n", psessiontuples->prot);
}

#if 0
//initialize session locker
spinlock_t *initsessionlock(void)
{
	spinlock_t *plock = NULL;

	plock = (spinlock_t *)kmalloc(sizeof(spinlock_t), GFP_KERNEL);
	*plock = SPIN_LOCK_UNLOCKED;
	//static DEFINE_SPINLOCK(plock);
	if (plock != NULL)	
		spin_lock_init(plock);
	
	return plock;
}

#endif
//session locker
void sessionlock(spinlock_t * lock)
{
	if (lock != NULL)
		spin_lock(lock);
		
}

//session unlocker
void sessionunlock(spinlock_t * lock)
{
	if (lock != NULL)
		spin_unlock(lock);
		
}

