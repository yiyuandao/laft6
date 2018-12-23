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


#ifndef PKT_UP_H_CPE
#define PKT_UP_H_CPE

#include "precomp.h"

int get_hash_data_up(struct sk_buff *skb, PHASH_DATA_UP up); 
int get_hash_data_down(struct sk_buff *skb, PHASH_DATA_DOWN down);
PSESSIONTUPLES get_tuple(PHASH_DATA_UP up, PHASH_DATA_DOWN down);

int compare_session(void *pKey, void *pData, int type);
PSESSIONTUPLES search_nat_hashtable(struct sk_buff* skb, PHASH_DATA_UP up);
int encap(struct sk_buff *skb, PSESSIONTUPLES session_tuple, unsigned int mtu);
int normal_packet_handl(struct sk_buff *skb, PSESSIONTUPLES session_tuple);
int single_packet_handl(struct sk_buff *skb, PSESSIONTUPLES session_tuple, unsigned int mtu);
int ipv6_frag_handl(struct sk_buff *skb, PSESSIONTUPLES session_tuple, unsigned int mtu);

#endif
