/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  thread relevant definitions                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_THREAD_QUE_H)
#define  _KERN_THREAD_QUE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>
#include <kern/queue.h>

/** スレッドキュー
 */
typedef struct _thread_queue{
	spinlock    lock;  /*< ロック変数              */
	queue        que;  /*< スレッドキューのヘッド  */
	obj_cnt_type cnt;  /*< キュー内のスレッド数    */
}thread_queue;

/** スレッドキュー初期化子
    @param[in] _que 初期化するキューのアドレス
 */
#define __TQ_INITIALIZER(_que)					\
	{							\
	.lock=__SPINLOCK_INITIALIZER,			        \
	.que = __QUEUE_INITIALIZER(_que),	                \
	.cnt = 0,                                               \
	}

struct _thread;
int tq_add(thread_queue *_q, struct _thread *_thr);
int tq_del(thread_queue *_q, struct _thread *_thr);
int tq_get_top(thread_queue *_q, struct _thread **_thrp);
bool tq_is_empty(thread_queue *_q);
void tq_init(thread_queue *_q);
#endif  /*  _KERN_THREAD_QUE_H   */
