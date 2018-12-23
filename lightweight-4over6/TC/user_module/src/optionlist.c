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
#include <stdlib.h>
#include "optionlist.h"
#include "opcode.h"
#include "log.h"

POPTIONLIST phead = NULL;

static POPTIONLIST option_new(void)
{
	POPTIONLIST poption =  NULL;

	poption = (POPTIONLIST)malloc(sizeof(*poption));
	if (poption != NULL)
	{
		memset(poption, 0, sizeof(*poption));
		return poption;
	}
	else
	{
		Log(LOG_LEVEL_ERROR, "low mem... quit now");
		exit(-1);
	}
}

static void list_traver(POPTIONLIST head, 
		HANDLER handle, void *data)
{
	POPTIONLIST p = head;

	for (; p != NULL; p = p->next)
	{
		handle(p, data);
	}
}

POPTIONLIST option_list_add(POPTIONLIST head, unsigned short opcode, 
			void *data)
{
	POPTIONLIST p = option_new();

	p->opcode = opcode;
	p->data = data;

	if (head != NULL)
	{
		p->next = head;
		head = p;
	}
	else
	{
		p->next = NULL;
		head = p;
	}

	return head;
}

POPTIONLIST option_list_search_opcode(POPTIONLIST head, unsigned short opcode)
{
	POPTIONLIST p = head;

	for (; p != NULL; p = p->next)
	{
		if (p->opcode == opcode)	
		{
			return p;
		}
	}

	return NULL;
}

int option_list_search_option(POPTIONLIST head, unsigned short option)
{
	POPCODE_T popcode = (POPCODE_T)head->data;
	int i = 0;

	// optionarr is init with 0xff
	do
	{
		if (popcode->optionarr[i] == option)
		{
			Log(LOG_LEVEL_NORMAL, "find option: %d", option);
			return 0;
		}
		else
		{
			i++;
		}
	}while (popcode->optionarr[i] != 0xffff);

	Log(LOG_LEVEL_NORMAL, "not find option: %d", option);
	return -1;
}

void option_list_print(POPTIONLIST head, HANDLER handle, void *ctx)
{
	list_traver(head, handle, ctx);
}

void option_list_free(POPTIONLIST head)
{
	POPTIONLIST next = head->next;

	while (head != NULL)
	{
		free(head->data);
		free(head);

		head = next;
	}
}
