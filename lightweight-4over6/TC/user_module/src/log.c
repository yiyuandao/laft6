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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

LOG_CONFIG logconfig;


/** 
 * @fn    int OpenLog(unsigned char *logroot, unsigned char flag)
 * @brief open log file
 * 
 * @param[in] logroot log file root
 * @param[in] flag output dir
 * 
 * @retval 1  success 
 * @retval -1 Failure 
 */
int OpenLog(char *logroot, unsigned char flag)
{
	int ret = 0;
	
	logconfig.flag = flag;
	memset(logconfig.logbuf, 0, sizeof(logconfig.logbuf));
	if (flag & OUTPUT_FILE)
	{
		logconfig.logfp = fopen(logroot, "a+");
		if (logconfig.logfp == NULL)
			return -1;
	}
	
	return 1;
}



/** 
 * @fn   void CloseLog(void)
 * @brief close log file
 * 
 * @param[in] NULL
 * 
 */
void CloseLog(void)
{
	if (logconfig.flag & OUTPUT_FILE)
	{
		if (logconfig.logfp != NULL)
			fclose(logconfig.logfp);
	}
}


