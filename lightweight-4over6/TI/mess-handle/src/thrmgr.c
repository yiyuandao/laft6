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

/*
 *  Copyright (C) 2004 Trog <trog@clamav.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>

#include "thrmgr.h"

#define FALSE (0)
#define TRUE  (1)

work_queue_t *work_queue_new()
{
	work_queue_t *work_q;

	work_q = (work_queue_t *) mmalloc(sizeof(work_queue_t));

	work_q->head = work_q->tail = NULL;
	work_q->item_count = 0;
	return work_q;
}

void work_queue_add(work_queue_t *work_q, void *data)
{
	work_item_t *work_item;

	if (!work_q) {
		return;
	}
	work_item = (work_item_t *) mmalloc(sizeof(work_item_t));
	work_item->next = NULL;
	work_item->data = data;
	gettimeofday(&(work_item->time_queued), NULL);

	if (work_q->head == NULL) {
		work_q->head = work_q->tail = work_item;
		work_q->item_count = 1;
	} else {
		work_q->tail->next = work_item;
		work_q->tail = work_item;
		work_q->item_count++;
	}
	return;
}

void *work_queue_pop(work_queue_t *work_q)
{
	work_item_t *work_item;
	void *data;

	if (!work_q || !work_q->head) {
		return NULL;
	}
	work_item = work_q->head;
	data = work_item->data;
	work_q->head = work_item->next;
	if (work_q->head == NULL) {
		work_q->tail = NULL;
	}
	free(work_item);
	return data;
}

void thrmgr_destroy(threadpool_t *threadpool)
{
	if (!threadpool || (threadpool->state != POOL_VALID)) {
		return;
	}
  	if (pthread_mutex_lock(&threadpool->pool_mutex) != 0) {
   		LOGE("!Mutex lock failed\n");
        exit(-1);
	}
	threadpool->state = POOL_EXIT;

	/* wait for threads to exit */
	if (threadpool->thr_alive > 0) {
        // 广播告知大家全部退出各自的工作,老子要死了
		if (pthread_cond_broadcast(&(threadpool->pool_cond)) != 0) {
			pthread_mutex_unlock(&threadpool->pool_mutex);
			return;
		}
	}
	while (threadpool->thr_alive > 0) {
        // 等着最后一个worker退出时
        // 执行pthread_cond_broadcast(&threadpool->pool_cond);来唤醒这里[luther.gliethttp]
		if (pthread_cond_wait (&threadpool->pool_cond, &threadpool->pool_mutex) != 0) {
			pthread_mutex_unlock(&threadpool->pool_mutex);
			return;
		}
	}
  	if (pthread_mutex_unlock(&threadpool->pool_mutex) != 0) {
        LOGE("!Mutex unlock failed\n");
        exit(-1);
  	}

	pthread_mutex_destroy(&(threadpool->pool_mutex));
	pthread_cond_destroy(&(threadpool->pool_cond));
	pthread_attr_destroy(&(threadpool->pool_attr));
	free(threadpool);
	return;
}

// 初始化线程池参数
// 1. max_threads   -- 最大线程数
// 2. idle_timeout  -- 线程空闲等待时间,如idle_timeout秒还没有人使用本worker线程,那么本线程将自行销毁[luther.gliethttp]
threadpool_t *thrmgr_new(int max_threads, int idle_timeout, void (*handler)(void *))
{
	threadpool_t *threadpool;

	if (max_threads <= 0) {
		return NULL;
	}

	threadpool = (threadpool_t *) mmalloc(sizeof(threadpool_t));

	threadpool->queue = work_queue_new();
	if (!threadpool->queue) {
		free(threadpool);
		return NULL;
	}
	threadpool->thr_max = max_threads;
	threadpool->thr_alive = 0; // 没有一个创建的线程
	threadpool->thr_idle = 0;  // 没有一个空闲的线程
	threadpool->idle_timeout = idle_timeout;
	threadpool->handler = handler; // 线程池消化threadpool->queue任务池上数据时使用到的统一消化函数[luther.gliethttp]

	pthread_mutex_init(&(threadpool->pool_mutex), NULL);
	if (pthread_cond_init(&(threadpool->pool_cond), NULL) != 0) {
		free(threadpool);
		return NULL;
	}

	if (pthread_attr_init(&(threadpool->pool_attr)) != 0) {
		free(threadpool);
		return NULL;
	}

	if (pthread_attr_setdetachstate(&(threadpool->pool_attr), PTHREAD_CREATE_DETACHED) != 0) {
		free(threadpool);
		return NULL;
	}
	threadpool->state = POOL_VALID;

	return threadpool;
}

// 线程池中一个线程,每次向线程池中追加一个线程就会使得统计变量threadpool->thr_alive++
// 对统计变量进行++加加操作是在用户程序使用thrmgr_dispatch派发新数据到threadpool->queue任务池时
// 自动完成的
void *thrmgr_worker(void *arg)
{
	threadpool_t *threadpool = (threadpool_t *) arg;
	void *job_data;
	int retval, must_exit = FALSE;
	struct timespec timeout;
	/* loop looking for work */
	for (;;) {
		if (pthread_mutex_lock(&(threadpool->pool_mutex)) != 0) { // 获取锁
			/* Fatal error */
			LOGE("!Fatal: mutex lock failed\n");
			exit(-2);
		}
		timeout.tv_sec = time(NULL) + threadpool->idle_timeout;
		timeout.tv_nsec = 0;
		threadpool->thr_idle++;
        // 开始饿狗抢食啦[luther.gliethttp]
		while (((job_data=work_queue_pop(threadpool->queue)) == NULL) // 从任务池中争抢一个任务来干[luther.gliethttp]
				&& (threadpool->state != POOL_EXIT)) {                // 因为如果线程池中有很多任务的话可能有多个线程
			/* Sleep, awaiting wakeup */                              // 来争抢来做这些任务.
			retval = pthread_cond_timedwait(&(threadpool->pool_cond),
				&(threadpool->pool_mutex), &timeout); // 如果没有任务可做,那么将等待thrmgr_new线程池默认的idle_timeout秒
			if (retval == ETIMEDOUT) {  // 如果idle_timeout秒之内,仍然没有人唤醒我做事
				LOGE("thrmgr_worker: get task timeout\n");
				must_exit = TRUE;       // (或者因为线程池中线程太多被其他线程池线程都抢走了)那么我将强行把自己退出
				break;
			}
		}
		threadpool->thr_idle--;         // 表示线程池中的空闲线程少了一个
		if (threadpool->state == POOL_EXIT) {
			must_exit = TRUE;
		}

		if (pthread_mutex_unlock(&(threadpool->pool_mutex)) != 0) {
			/* Fatal error */
			LOGE("!Fatal: mutex unlock failed\n");
			exit(-2);
		}
		if (job_data) {
			threadpool->handler(job_data); // 本线程消化threadpool->queue任务池上该job_data数据时使用到的统一消化函数
		} else if (must_exit) {
			break;                      // 自己已经idle_timeout秒没有事做了,自行强行销毁
		}
	}

	if (pthread_mutex_lock(&(threadpool->pool_mutex)) != 0) {
		/* Fatal error */
		LOGE("!Fatal: mutex lock failed\n");
		exit(-2);
	}
	threadpool->thr_alive--;            // 线程池中可用线程少了一个,可能是idle_timeout秒没有事做,
	if (threadpool->thr_alive == 0) {   // 也可能是thrmgr_destroy强行退出
		/* signal that all threads are finished */
		pthread_cond_broadcast(&threadpool->pool_cond); // 如果是thrmgr_destroy强行退出,就必须执行该broadcast
	}                                                   // 否则thrmgr_destroy将因为pthread_cond_wait而一直等待下去[luther.gliethttp]
	if (pthread_mutex_unlock(&(threadpool->pool_mutex)) != 0) {
		/* Fatal error */
		LOGE("!Fatal: mutex unlock failed\n");
		exit(-2);
	}
	return NULL;
}

// 用户程序使用thrmgr_dispatch派发新数据到threadpool->queue任务池
int thrmgr_dispatch(threadpool_t *threadpool, void *user_data)
{
	pthread_t thr_id;

	if (!threadpool) {
		return FALSE;
	}

	/* Lock the threadpool */
	if (pthread_mutex_lock(&(threadpool->pool_mutex)) != 0) {
		LOGE("!Mutex lock failed\n");
		return FALSE;
	}

	if (threadpool->state != POOL_VALID) {
		if (pthread_mutex_unlock(&(threadpool->pool_mutex)) != 0) {
			LOGE("!Mutex unlock failed\n");
			return FALSE;
		}
		return FALSE;
	}

//	LOGE(" add a new task\n");
	work_queue_add(threadpool->queue, user_data); // 向threadpool->queue任务池推入一个新任务

	if ((threadpool->thr_idle == 0) && // 当前没有一个idle空闲的线程(因为他们都在做事或者本来就还一个都没有创建)
			(threadpool->thr_alive < threadpool->thr_max)) { // 同时已经创建的线程数目没有超过默认最大数量
		/* Start a new thread */
		if (pthread_create(&thr_id, &(threadpool->pool_attr),// ok, 那么我们就再创建一个新线程到线程池中
				thrmgr_worker, threadpool) != 0) {
			LOGE("!pthread_create failed\n");
		} else {
			threadpool->thr_alive++; // 线程池中又多了一个可用的做事线程
		}
	}

	pthread_cond_signal(&(threadpool->pool_cond)); // 唤醒刚刚做完事的线程让他继续做事,如果没有,那么上面创建的线程将会做该事

	if (pthread_mutex_unlock(&(threadpool->pool_mutex)) != 0) {
		LOGE("!Mutex unlock failed\n");
		return FALSE;
	}
	return TRUE;
}
