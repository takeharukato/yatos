/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Timer service relevant definitions                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_TIMER_H)
#define  _KERN_TIMER_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>
#include <kern/rbtree.h>
#include <kern/thread-sync.h>

/**  タイマキュー
 */
typedef struct _timer_queue{
	spinlock                              lock;  /*< タイマーキューのロック */
	RB_HEAD(timer_queue, _timer_callout)  que;  /*< タイマキューののヘッド  */
}timer_queue;

#define __TIMER_QUEUE_INITIALIZER(root)  {  \
	.lock = __SPINLOCK_INITIALIZER,     \
	.que = RB_INITIALIZER(root),        \
	}

/** タイマコールバック
 */
typedef struct _timer_callout{
	RB_ENTRY(_timer_callout)  tnode;  /*< 赤黒木のノード                         */
	tim_tmout                 tmout;  /*< 一回当たりのタイムアウト時間(単位:ms)  */
	ticks                    expire;  /*< 次回発火時のtick値                     */
	void   (*callout)(private_inf );  /*< コールアウト関数                       */
	private_inf                data;  /*< コールアウト関数に渡す引数             */
}timer_callout;

/** ティック管理
 */
typedef struct _uptime_ticks{
	spinlock  lock;  /*< アップタイムのロック            */
	ticks tick_cnt;  /*< 電源投入後の総ティック発生回数  */
}uptime_ticks;

int kcom_tim_register_timer_irq(intr_no _no, private_inf _data);
int kcom_tim_unregister_timer_irq(intr_no _no);
void tim_init_subsys(void);

sync_reason tim_wait(tim_tmout _outms);
sync_reason tim_wait_obj(struct _sync_obj *_obj, tim_tmout _outms);
void mdelay(delay_cnt _ms);
void udelay(delay_cnt _us);

void hal_timer_init(void);
void hal_setup_udelay(void);
void hal_udelay(delay_cnt _us);
#endif  /*  _KERN_TIMER_H   */
