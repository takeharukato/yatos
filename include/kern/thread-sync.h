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
#include <kern/thread-state.h>

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

/** コールバック呼び出し要因
 */
typedef enum _sync_callback_reason{
	SYNC_WAIT_CALL_WAIT = 0,  /*<  休眠前(ロック解除)          */
	SYNC_WAIT_CALL_WAKE = 1,   /*<  起床後(ロック獲得)          */
}sync_callback_reason;

struct _thread;

typedef void * sync_callback_arg; /*< コールバック引数 */
/** コールバック関数
 */
typedef void (*sync_callback)(enum _sync_callback_reason _reason, sync_callback_arg _arg);

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
	spinlock        lock;  /*< スピンロック         */
	thr_state  wait_kind;  /*< オブジェクト休眠種別 */
	sync_pol      policy;  /*< スレッド起床方針     */
	queue            que;  /*< キュー本体           */
}sync_obj;

#define __SYNC_OBJECT_INITIALIZER(_sobj, _pol, _wait_kind)	\
	{							\
	.lock=__SPINLOCK_INITIALIZER,			        \
	.wait_kind = (_wait_kind),		                \
	.policy = _pol,				                \
	.que = __QUEUE_INITIALIZER(&(_sobj.que)),	        \
	}

void sync_spinlocked_callback(sync_callback_reason _reason, void *_arg);
sync_reason sync_wait_with_callback(sync_obj *_obj, sync_callback _callback,
				    sync_callback_arg _arg);
void sync_init_object(sync_obj *_obj, sync_pol _pol, thr_state _wait_kind);
sync_reason sync_wait(sync_obj *_obj, spinlock *_lock);
void sync_wake(sync_obj *_obj, sync_reason _reason);
#endif  /*  _KERN_THREAD_SYNC_H   */
