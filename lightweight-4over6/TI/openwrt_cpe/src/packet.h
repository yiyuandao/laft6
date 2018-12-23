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


#ifndef _PACKET_H_
#define _PACKET_H_

#define MINS * 60 
#define HOURS * 60 MINS
#define DAYS * 24 HOURS

#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20
#define TH_ECE  0x40
#define TH_CWR  0x80

static unsigned long tcp_timeouts[]
= { 
	30 MINS, 	/*	TCP_CONNTRACK_NONE,	*/
	//5 DAYS,	/*	TCP_CONNTRACK_ESTABLISHED,	*/
	8 MINS,	/*	TCP_CONNTRACK_ESTABLISHED,	*/
	2 MINS,	/*	TCP_CONNTRACK_SYN_SENT,	*/
	60 ,	/*	TCP_CONNTRACK_SYN_RECV,	*/
	2 MINS,	/*	TCP_CONNTRACK_FIN_WAIT,	*/
	2 MINS,	/*	TCP_CONNTRACK_TIME_WAIT,	*/
	10 ,	/*	TCP_CONNTRACK_CLOSE,	*/
	60 ,	/*	TCP_CONNTRACK_CLOSE_WAIT,	*/
	30 ,	/*	TCP_CONNTRACK_LAST_ACK,	*/
	2 MINS,	/*	TCP_CONNTRACK_LISTEN,	*/
};

#define UDP_TIMEOUT 30 
#define UDP_STREAM_TIMEOUT 180

#define ICMP_TIMEOUT 30


enum tcp_conntrack { 
	TCP_CONNTRACK_NONE, 
	TCP_CONNTRACK_ESTABLISHED, 
	TCP_CONNTRACK_SYN_SENT, 
	TCP_CONNTRACK_SYN_RECV, 
	TCP_CONNTRACK_FIN_WAIT, 
	TCP_CONNTRACK_TIME_WAIT, 
	TCP_CONNTRACK_CLOSE, 
	TCP_CONNTRACK_CLOSE_WAIT, 
	TCP_CONNTRACK_LAST_ACK, 
	TCP_CONNTRACK_LISTEN, 
	TCP_CONNTRACK_MAX 
}; 

enum ip_conntrack_dir 
{ 
	IP_CT_DIR_ORIGINAL, 
	IP_CT_DIR_REPLY, 
	IP_CT_DIR_MAX 
}; 

#define sNO 	TCP_CONNTRACK_NONE
#define sES 	TCP_CONNTRACK_ESTABLISHED
#define sSS 	TCP_CONNTRACK_SYN_SENT
#define sSR 	TCP_CONNTRACK_SYN_RECV
#define sFW 	TCP_CONNTRACK_FIN_WAIT
#define sTW 	TCP_CONNTRACK_TIME_WAIT
#define sCL 	TCP_CONNTRACK_CLOSE
#define sCW 	TCP_CONNTRACK_CLOSE_WAIT
#define sLA 	TCP_CONNTRACK_LAST_ACK
#define sLI 	TCP_CONNTRACK_LISTEN
#define sIV 	TCP_CONNTRACK_MAX

#define PKT_DOWN 	1 
#define PKT_UP		0


static enum tcp_conntrack tcp_conntracks[2][5][TCP_CONNTRACK_MAX] = {
	{
		/*	ORIGINAL */
		/* 	  sNO, sES, sSS, sSR, sFW, sTW, sCL, sCW, sLA, sLI 	*/
		/*syn*/	{sSS, sES, sSS, sSR, sSS, sSS, sSS, sSS, sSS, sLI },
		/*fin*/	{sTW, sFW, sSS, sTW, sFW, sTW, sCL, sTW, sLA, sLI },
		/*ack*/	{sES, sES, sSS, sES, sFW, sTW, sCL, sCW, sLA, sES },
		/*rst*/ {sCL, sCL, sSS, sCL, sCL, sTW, sCL, sCL, sCL, sCL },
		/*none*/{sIV, sIV, sIV, sIV, sIV, sIV, sIV, sIV, sIV, sIV }
	},
	{
		/*	REPLY */
		/* 	  sNO, sES, sSS, sSR, sFW, sTW, sCL, sCW, sLA, sLI 	*/
		/*syn*/	{sSR, sES, sSR, sSR, sSR, sSR, sSR, sSR, sSR, sSR },
		/*fin*/	{sCL, sCW, sSS, sTW, sTW, sTW, sCL, sCW, sLA, sLI },
		/*ack*/	{sCL, sES, sSS, sSR, sFW, sTW, sCL, sCW, sCL, sLI },
		/*rst*/ {sCL, sCL, sCL, sCL, sCL, sCL, sCL, sCL, sLA, sLI },
		/*none*/{sIV, sIV, sIV, sIV, sIV, sIV, sIV, sIV, sIV, sIV }
	}
};

// Á¬½Ó×´Ì¬.  
// Bitset representing status of connection.  
enum ip_conntrack_status  
{ 
	// It's an expected connection: bit 0 set.  This bit never changed  
	IPS_EXPECTED_BIT = 0, 
	IPS_EXPECTED = (1 << IPS_EXPECTED_BIT), 

	// We've seen packets both ways: bit 1 set.  Can be set, not unset.  
	IPS_SEEN_REPLY_BIT = 1, 
	IPS_SEEN_REPLY = (1 << IPS_SEEN_REPLY_BIT), 

	// Conntrack should never be early-expired. 
	IPS_ASSURED_BIT = 2, 
	IPS_ASSURED = (1 << IPS_ASSURED_BIT), 
}; 

#define N 2000
#define BITSPERWORD 32
#define SHIFT 5
#define MASK 0x1F

typedef struct _tag_tcp_cb 
{ 
	enum tcp_conntrack state; 
	unsigned long handshake_ack;
}TCP_CB, *PTCP_CB;

typedef struct _tag_sessiontuples
{
	unsigned int srcip;
	unsigned int dstip;
	unsigned short oldport;
	unsigned short newport;
	unsigned char prot;

	 short icmp_seq;
	TCP_CB tcp_cb;
	unsigned char con_status;
	long time_out;
}SESSIONTUPLES, *PSESSIONTUPLES;

#pragma pack(1)
typedef struct hash_data_up
{
	unsigned int srcip;
	unsigned int dstip;
	unsigned short oldport;
	unsigned char proto;
}HASH_DATA_UP, *PHASH_DATA_UP;

typedef struct hash_data_down
{
	unsigned int dstip;
	unsigned short newport;
	unsigned char proto;
}HASH_DATA_DOWN, *PHASH_DATA_DOWN;
#pragma pack()

//#define SENDSKB(n) Dev_queue_xmit(n)
void Dev_queue_xmit(struct sk_buff *skb);
//#define SENDSKB(n) netif_rx(n)
int tcp_pkt_track(struct sk_buff* skb,
		PSESSIONTUPLES ptuples,
		unsigned char  maniptype);

void udp_pkt_track(PSESSIONTUPLES ptuples);

void icmp_pkt_track(PSESSIONTUPLES ptuples, 
		unsigned char maniptype);

void setportbit(PSESSIONTUPLES psessiontuples);
void
patch_tcpmss(unsigned char *buf4, unsigned int len);
#endif
