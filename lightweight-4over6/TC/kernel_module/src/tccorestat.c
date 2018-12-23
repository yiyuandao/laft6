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
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include "tccorestat.h"

TCCORESTATINFO_T TCCORESTATINFO;
spinlock_t 			TcCoreStatLock;

void InitTCCoreStatInfo(void)
{	
	spin_lock_init(&TcCoreStatLock);
	memset(&TCCORESTATINFO, 0, sizeof(TCCORESTATINFO_T));
}

void SetTCCoreStatInfo(STAT_TYPE stat_type, unsigned int size)
{
	spin_lock(&TcCoreStatLock);
	TCCORESTATINFO.llStat[stat_type] += size;
	spin_unlock(&TcCoreStatLock);
}

long long GetTCCoreStatInfo(STAT_TYPE stat_type)
{	
	long long temp = 0; 
	spin_lock(&TcCoreStatLock);
	temp = TCCORESTATINFO.llStat[stat_type];
	spin_unlock(&TcCoreStatLock);

	return temp;
}

void DelTCCoreStatInfo(STAT_TYPE stat_type, unsigned int size)
{
	spin_lock(&TcCoreStatLock);
	TCCORESTATINFO.llStat[stat_type] -= size;
	spin_unlock(&TcCoreStatLock);
}

void CopyTCCoreStatInfo(PTCCORESTATINFO_T pinfo)
{
	spin_lock(&TcCoreStatLock);
	memcpy(pinfo, &TCCORESTATINFO, sizeof(TCCORESTATINFO));
	spin_unlock(&TcCoreStatLock);
}

