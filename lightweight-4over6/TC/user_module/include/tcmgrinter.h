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
#ifndef _TCMGRINTER_H_
#define _TCMGRINTER_H_


#define HDR_FLAG 			0xff

#define CMD_TYPE_STATE_REQ 		0x01
#define CMD_TYPE_STATE_REPLY 		0x02


typedef enum 
{
	USER_SIZE = 0,
	TX_BYTE,
	RX_BYTE,
	TX_PACKET,
	RX_PACKET,
	DROP_TX_BYTE,
	DROP_RX_BYTE,
	DROP_TX_PACKET,
	DROP_RX_PACKET,
	STAT_MAX
}STAT_TYPE;


typedef struct _tag_tccorestatinfo_t
{
	long long  				llStat[STAT_MAX];
}TCCORESTATINFO_T, *PTCCORESTATINFO_T;

typedef struct _tag_inter_hdr
{
	unsigned char flaghdr;
	unsigned char cmdtype;
}INTER_HDR, *PINTER_HDR;

typedef struct _tag_intermsg
{
	INTER_HDR interhdr;
	char msg[1024];
}INTER_MSG, *PINTER_MSG;

int InitInterSocket(void);

int UnInitInterSocket(void);

void StatesReply(PINTER_MSG pstatereply);

void ThreadInterfaceProc(void *param);
#endif
