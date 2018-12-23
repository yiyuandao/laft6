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


#ifndef _PCPPORTSETMSGDEF_H_
#define _PCPPORTSETMSGDEF_H_
#include <asm/byteorder.h>

//pcp server port
#define PCP_PORT 5351

//error code 
enum
{
	ERR_SUCCESS = 0,
	ERR_UNSUPP_VERSION,
	ERR_NOT_AUTHORIZED,
	ERR_MALFORMED_REQUEST,
	ERR_UNSUPP_OPCODE,
	ERR_UNSUPP_OPTION,
	ERR_MALFORMED_OPTION,
	ERR_NETWORK_FAILURE,
	ERR_NO_RESOURCES,
	ERR_UNSUPP_PROTOCOL,
	ERR_USER_EX_QUOTA,
	ERR_CANNOT_PROVIDE_EXTERNAL,
	ERR_ADDRESS_MISMATCH,
	ERR_EXCESSIVE_REMOTE_PEERS,
	ERR_MAX
};

//pcp version
#define PCP_VER		0x01
//opcode
#define OP_CODE_MAP_PORT_SET	100
//pcp request
#define PCP_REQ					0x00
//pcp response
#define PCP_RESP				0x01

//pcp maximum retransmission time(units:second)
#define MAX_RETRANSTIME			512
//pcp initialize retransmission time(units:second)
#define INIT_RETRANSITME		2
//pcp maximum message length
#define MAX_PCPPORTSET_MSG_LEN	1024

#pragma  pack(1)
typedef struct _tag_opcode_mps_req_t
{
	unsigned  int	uiProtocolandReserved;//protocol:8bit and reserved 24bit
	unsigned char	aucSEIPAddr[16];
}OPCODE_MPS_REQ_T, *POPCODE_MPS_REQ_T;

typedef struct _tag_op_mps_rsp_t
{
	unsigned  int	uiProtocolandReserved;//protocol:8bit and reserved 24bit
	unsigned char	aucAEIPAddr[16];
}OPCODE_MPS_RSP_T, *POPCODE_MPS_RSP_T;

typedef struct _tag_option_mps_t
{
	unsigned char ucOptionCode;
	unsigned char ucReserved;
	unsigned short usOptionLen;
	unsigned short usPortRangeValue;
	unsigned short usPortRangeMask;
}OPTION_MPS_T, *POPTION_MPS_T;

typedef struct _tag_pcpreq_t
{
	unsigned char ucVer;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	unsigned char ucOpCode:7, ucR:1;
#elif defined (__BIG_ENDIAN_BITFIELD)
	unsigned char ucR:1, ucOpCode:7;
#endif

	unsigned short  uiReserved;
	unsigned int  uiReqLiftTime;
	unsigned char aucCLTIPv6Addr[16];
	OPCODE_MPS_REQ_T mps_req_t;
	OPTION_MPS_T  option_mps_t;
}PCP_REQ_T, *PPCP_REQ_T;

typedef struct _tag_pcprsp_t
{
	unsigned char		ucVer;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	unsigned char ucOpCode:7, ucR:1;
#elif defined (__BIG_ENDIAN_BITFIELD)
	unsigned char ucR:1, ucOpCode:7;
#endif
	unsigned char		ucReserved;
	unsigned char		ucResultCode;
	unsigned int		uiReqLiftTime;
	unsigned int		uiEpochTime;
	unsigned char		aucReserved[12];
	OPCODE_MPS_RSP_T	mps_rsp_t;
	OPTION_MPS_T		option_mps_t;
}PCP_RSP_T, *PPCP_RSP_T;
#pragma  pack()


#endif
