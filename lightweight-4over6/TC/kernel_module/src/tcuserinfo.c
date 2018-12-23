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
#include "tcuserinfo.h"
#include "netlinkmsgdef.h"
#include "netlink.h"
#include "bplus.h"
#include "global.h"
#include "log.h"

static int flag = 0;
static TCUSERINFO msg = {
			.msghdr = {
				.msgdef = NETLINK_TC_USER_INFO
			},
};

static void send_user_info(void *record)
{
	static int i = 0;
	static int offset = 0;
	
	if (flag == 1)
	{
		offset = 0;
		i = 0;

		flag = 0;
	}

	if (i < 49)
	{
		// copy user info
		memcpy(&msg.user[i], record, sizeof(USERINFO));

		offset += sizeof(USERINFO);
		i++;
	}
	else
	{
		memcpy(&msg.user[i], record, sizeof(USERINFO));
		offset += sizeof(USERINFO);

		msg.msglen = 50 * sizeof(USERINFO);
		send_tc_info_nlmsg((char *)&msg, offset + 3 * sizeof(unsigned int));

		Log(LOG_LEVEL_NORMAL, "-------------send all 50 item");

		offset = 0;
		i = 0;

	}
}

int BPTravUserInfo(struct BPTree *up)
{
   struct BPValue* curValue = NULL,*next;

   /* validation */
   if(up == NULL)
	   return -1;

   if((up->root == NULL)||(up->header == NULL))
	   return 0;

   curValue = up->header;

   while(curValue != NULL)
   {
	 next = curValue->next;
	 //spin_lock(&GlobalCtx.StorageLock);
	 send_user_info(curValue->pRecord);
	 //spin_unlock(&GlobalCtx.StorageLock);
	 curValue = next;
   }
 
   return 0;
}
void process_user_info(char *req)
{
	static int nelem = 0;
	int nleft = 0;
	PNETLINK_MSG_HDR pmsg = (PNETLINK_MSG_HDR)req;
#if 0
	TCUSERINFO user = {
		.msghdr = {
			.msgdef = NETLINK_TC_USER_TOTAL
		},
		.total_user = nelem
	};
#endif
	struct user
	{
		NETLINK_MSG_HDR 		msghdr;
		unsigned int 			total_user;
	}user;

	Log(LOG_LEVEL_NORMAL, "+++++++++++----sart now---------m");

	switch (pmsg->msgdef)
	{
	case  NETLINK_TC_USER_TOTAL:
		{	
			nelem = SizeOfElement(GlobalCtx.pStroageCtx);
			user.msghdr.msgdef =  NETLINK_TC_USER_TOTAL;
			user.total_user = nelem;
			// send total user
			Log(LOG_LEVEL_NORMAL, "SizeOfElement :%d", nelem);
			send_tc_info_nlmsg((char *)&user, sizeof(NETLINK_MSG_HDR) + sizeof(unsigned int));
		}
		break;

	case  NETLINK_TC_USER_INFO:
		{
			// send user info
			BPTravUserInfo(&GlobalCtx.pStroageCtx->BPTreeCtx[ELEMENT_4]);

			if ( (nleft = nelem % 50))
			{
				Log(LOG_LEVEL_NORMAL, "+++++++++++-------------send %d item", nleft);
				msg.msglen = nleft * sizeof(USERINFO);
				send_tc_info_nlmsg((char *)&msg, nleft * sizeof(USERINFO) + 3 * sizeof(unsigned int));
				flag = 1;
			}

			// send ok
			user.msghdr.msgdef = NETLINK_RESULT;
			send_tc_info_nlmsg((char *)&user, sizeof(NETLINK_MSG_HDR));
		}
		break;
	default:
		Log(LOG_LEVEL_WARN, "unknow request");
		break;

	}
}
