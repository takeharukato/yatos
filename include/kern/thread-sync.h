/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Thread synchronization relevant definitions                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_THREAD_SYNC_H)
#define  _KERN_THREAD_SYNC_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/errno.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/queue.h>
#include <kern/list.h>
#include <kern/spinlock.h>

/** スレッド起床方針
 */
typedef enum _sync_pol{
	SYNC_WAKE_FLAG_ALL = 0,  /*<  待機中の全スレッドを起こす          */
	SYNC_WAKE_FLAG_ONE = 1,  /*<  キューの先頭スレッドをだけを起こす  */
}sync_pol;

/** 起床要因
 */
typedef enum _sync_reason{
	SYNC_WAI_WAIT = 0,       /*<  待ち状態                       */
	SYNC_WAI_RELEASED = 1,   /*<  待ち状態が解除された           */
	SYNC_OBJ_DESTROYED = 2,  /*<  対象オブジェクトが破棄された   */
	SYNC_WAI_TIMEOUT = 3,    /*<  タイムアウト                   */
	SYNC_WAI_DELIVEV = 4,    /*<  イベント受信                   */
}sync_reason;

struct _thread;
/** 同期ブロック
 */
typedef struct _sync_block{
	struct _thread  *thr;  /*< 待ち合わせているスレッド                         */
	list           olink;  /*< オブジェクトの待ちキューへのリンク               */
	sync_reason   reason;  /*< 起床要因                                         */
}sync_block;

/** 同期オブジェクト
 */
typedef struct _sync_obj{
	spinlock   lock;  /*< スピンロック      */
	sync_pol policy;  /*< スレッド起床方針  */
	queue       que;  /*< キュー本体        */
}sync_obj;

#define __WAIT_OBJECT_INITIALIZER(_que, _flags)			\
	{							\
	.lock=__SPINLOCK_INITIALIZER,			        \
	.wbflags = _flags,			                \
	.que = __QUEUE_INITIALIZER(_que),	                \
	}

void sync_init_object(sync_obj *_obj, sync_pol _pol);
sync_reason sync_wait(sync_obj *_obj);
void sync_wake(sync_obj *_obj, sync_reason _reason);
#endif  /*  _KERN_THREAD_SYNC_H   */
