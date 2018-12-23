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

#include <stdio.h>
#include <time.h>

#define OUTPUT_CONSOLE		0x01 //write console 
#define OUTPUT_FILE			0X02//write file
#define OUTPUT_ALL			0xff //console and file

#define MAX_LOG_LEN		1024

#define LOG_NAME		"runlog.log"
enum 
{
	LOG_LEVEL_NORMAL = 0,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR, 
	LOG_LEVEL_MAX
};

#define CURRENT_LOG_LEVEL LOG_LEVEL_NORMAL

typedef struct _tag_log
{
	unsigned char level;
	char log[10];
}LOG_LIST, *PLOG_LIST;

typedef struct _tag_logconfig
{
	FILE *logfp;
	unsigned char flag;
	char logbuf[MAX_LOG_LEN];
}LOG_CONFIG, *PLOG_CONFIG;

		
#define Log(level, fmt, ...)				\
do											\
{											\
	LOG_LIST LogList[LOG_LEVEL_MAX] = {		\
	{LOG_LEVEL_NORMAL, "[Normal]"},				\
	{LOG_LEVEL_WARN, "[Warn]"},				\
	{LOG_LEVEL_ERROR, "[Error]"}};			\
	char tmp[64] = {0};			\
	time_t now;							\
	struct tm *timenow;					\
	time(&now);							\
	timenow = localtime(&now);				\
	if (CURRENT_LOG_LEVEL <= (level))			\
	{										\
		if (logconfig.flag & OUTPUT_FILE)			\
		{										\
			sprintf(tmp, "%02d:%02d:%02d:", timenow->tm_hour, timenow->tm_min, timenow->tm_sec);\
			fwrite(tmp, strlen(tmp), 1, logconfig.logfp);	\
			sprintf(tmp, "[%s]", __FUNCTION__);	\
			fwrite(tmp, strlen(tmp), 1, logconfig.logfp);	\
			fwrite(LogList[(level)].log, strlen(LogList[(level)].log), 1, logconfig.logfp); \
			sprintf(logconfig.logbuf, fmt, ##__VA_ARGS__);	\
			fwrite(logconfig.logbuf, strlen(logconfig.logbuf), 1, logconfig.logfp);	\
			fwrite("\r\n", strlen("\r\n"), 1, logconfig.logfp);	\
		}										\
		if (logconfig.flag & OUTPUT_CONSOLE)		\
		{										\
			printf("%02d:%02d:%02d:", timenow->tm_hour, timenow->tm_min, timenow->tm_sec); \
			printf("[%s]%s:", (__FUNCTION__), LogList[(level)].log);	\
			printf(fmt, ##__VA_ARGS__);				\
			printf("\n");						\
		}								\
	}									\
} while(0)


extern LOG_CONFIG logconfig;

int OpenLog(char *logroot, unsigned char flag);
void CloseLog(void);
#endif
