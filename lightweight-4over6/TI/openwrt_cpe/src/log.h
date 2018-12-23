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


#ifndef _LOG_H_
#define _LOG_H_

enum 
{
	LOG_LEVEL_NORMAL = 0,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR, 
	LOG_LEVEL_MAX
};

#define CURRENT_LOG_LEVEL LOG_LEVEL_ERROR

typedef struct _tag_log
{
	unsigned char level;
	unsigned char log[10];
}LOG_LIST, *PLOG_LIST;

		
#define Log(level, fmt, ...)				\
do											\
{											\
	LOG_LIST LogList[LOG_LEVEL_MAX] = {		\
	{LOG_LEVEL_NORMAL, "[Normal]"},				\
	{LOG_LEVEL_WARN, "[Warn]"},				\
	{LOG_LEVEL_ERROR, "[Error]"}};			\
	struct timex  txc;						\
	struct rtc_time tm;						\
	if (CURRENT_LOG_LEVEL <= (level))			\
	{										\
		do_gettimeofday(&(txc.time));			\
		rtc_time_to_tm(txc.time.tv_sec,&tm);		\
		printk("%02d:%02d:%02d:", tm.tm_hour, tm.tm_min, tm.tm_sec); 		\
		printk("[%s]%s:", (__FUNCTION__), LogList[(level)].log);				\
		printk(fmt, ##__VA_ARGS__);				\
		printk("\n");						\
	}									\
} while(0)

#endif

