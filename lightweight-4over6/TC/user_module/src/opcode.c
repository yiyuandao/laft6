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
#include "optionlist.h"
#include "opcode.h"
#include "log.h"

#define	OPCODE			"opcode"
#define	OPTION			"option"

int load_opcode_file(char *filename)
{
	FILE *pf = NULL;
	char line[128] = {0};
	char *pos = NULL;
	int optindx = 0;
	unsigned short opcode = 0;
	int flag = 0;
	POPCODE_T popcode = NULL; 
	pf = fopen(filename, "r");
	if (pf == NULL) {
		Log(LOG_LEVEL_ERROR, "pf == NULL");
		return -1;
	}
	while (!feof(pf)) {
		memset(line, 0, sizeof(line));
		if (fgets(line, sizeof(line), pf) == NULL) {
			break;
		}
		if (line[0] == '#' || line[0] == '\r' || line[0] == '\n')
			continue;

		pos = strstr(line, OPCODE);
		if (pos != NULL) {
			pos = strtok(line, OPCODE);
			if (pos != NULL) {

				popcode = (POPCODE_T)malloc(sizeof(OPCODE_T));
				if (popcode == NULL)
					return -1;
				memset(popcode, 0xff, sizeof(OPCODE_T));
				opcode = atoi(pos + 1);
				flag = 1;
			}
		}
		pos = strstr(line, "{");
		if (pos != NULL) {
			optindx = 0;
			continue;
		}
		pos = strstr(line, "}");
		if (pos != NULL) {
			// insert list
			phead = option_list_add(phead, opcode, popcode);
			continue;
		}
		pos = strstr(line, OPTION);
		if (pos != NULL) {
			pos = strtok(pos, OPTION);
			if (pos != NULL) {
				if (popcode) {
					
					if (optindx >= MAX_OPTION_NUM)
					{
						Log(LOG_LEVEL_ERROR, "> MAX_OPTION_NUM");
						fclose(pf);
						return -1;
					}
					popcode->optionarr[optindx] = atoi(pos + 1);
					Log(LOG_LEVEL_NORMAL, "option: %d", popcode->optionarr[optindx]);
					optindx ++;
					
				}
			}
		}
	}

	if (flag == 0)
	{
		fclose(pf);
		return -1;
	}

	fclose(pf);

	return 1;
}
