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

#define MINI_TIMER_VALUE 5


//check session node timeout
int sessiondatproc(void *pdata)
{
	PSESSIONTUPLES ptuples = NULL;
	
	ptuples = (PSESSIONTUPLES)pdata;
	if (ptuples->time_out > 0)
	{
		ptuples->time_out -= MINI_TIMER_VALUE;
		if (ptuples->time_out  <= 0)
			return -1;
	}
	return 1;
}

//delete session node
void DataDel(void *pData)
{
	setportbit((PSESSIONTUPLES)pData);
	DBKeyHash_MemFree(pData);
}

//handle session timeout node
void sessiontmrhandle(unsigned long data)
{
	int i = 0;
	
	//Log(LOG_LEVEL_NORMAL, ("5s is timeout"));

	for (i = 0; i < pnathash->BucketCnt; i ++)
	{
		//delete the timeout session node
		sessionlock(&hashlock);
		DBKeyHashTableLoopDelete(pnathash,
			&pnathash->Bucket[i].node_link_head);
		sessionunlock(&hashlock);
	}

	sessiontimer.expires = jiffies + MINI_TIMER_VALUE * HZ;
 	add_timer(&sessiontimer);
}

//initialize session timer
void Initsessiontimer(void)
{
	init_timer(&sessiontimer);

	sessiontimer.data = 0;
	sessiontimer.function  = sessiontmrhandle;
	sessiontimer.expires = jiffies + MINI_TIMER_VALUE * HZ;
	add_timer(&sessiontimer);
}

//stop session timer
void stopsessiontimer(void)
{
	del_timer(&sessiontimer);
}
