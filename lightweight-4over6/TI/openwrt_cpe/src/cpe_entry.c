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


#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#include "precomp.h"

static struct nf_hook_ops stream_cfg_ops;
static struct nf_hook_ops stream_down_ops;
static struct nf_hook_ops stream_up_ops;


MODULE_LICENSE("GPL");

int start_cpe_module(void)
{
	Log(LOG_LEVEL_NORMAL, "ver1.0.3 2012/8/9 cpe kernel start..");
	
	modules_init();

	stream_cfg_ops.hook = stream_cfg_hook;
	stream_cfg_ops.pf = PF_INET6;
	stream_cfg_ops.hooknum = NF_INET_LOCAL_OUT;
	stream_cfg_ops.priority = NF_IP_PRI_FILTER + 2;	
	
	stream_up_ops.hook = stream_up_hook;
	stream_up_ops.pf = PF_INET;
	//stream_up_ops.hooknum = NF_INET_POST_ROUTING;
	stream_up_ops.hooknum = NF_INET_PRE_ROUTING;
	stream_up_ops.priority = NF_IP_PRI_FILTER + 2;
	
	stream_down_ops.hook = stream_down_hook;
	stream_down_ops.pf = PF_INET6;
	stream_down_ops.hooknum = NF_INET_PRE_ROUTING;
	stream_down_ops.priority = NF_IP_PRI_FILTER + 1;

	nf_register_hook(&stream_cfg_ops);
	nf_register_hook(&stream_up_ops);
	nf_register_hook(&stream_down_ops);
	
	return 0;
}

void clean_cpe_module(void)
{	
	nf_unregister_hook(&stream_down_ops);
	nf_unregister_hook(&stream_up_ops);
	nf_unregister_hook(&stream_cfg_ops);
	Log(LOG_LEVEL_NORMAL, "cpe kernel end...");
	modules_uninit();
}

module_init(start_cpe_module);
module_exit(clean_cpe_module);

