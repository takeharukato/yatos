/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Timer relevant routines                                           */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/errno.h>
#include <kern/spinlock.h>
#include <kern/thread.h>
#include <kern/timer.h>
#include <kern/sched.h>
#include <kern/async-event.h>

#include <thr/thr-internal.h>
#include <tim/tim-internal.h>

/*  タイマコールアウトキュー  */
static timer_queue timer_callout_queue =
	__TIMER_QUEUE_INITIALIZER( &timer_callout_queue.que ); 

static int timer_handler_cmp(struct _timer_callout *_a, struct _timer_callout *_b);

RB_GENERATE_STATIC(timer_queue, _timer_callout, tnode, timer_handler_cmp);

/** タイマコールバックの発火時間を比較する
    @param[in] a 比較対象のコールバック1
    @param[in] b 比較対象のコールバック2
    @retval 0  両者の発火時間が同じ
    @retval 負 コールバック1の発火時間のほうが前
    @retval 正 コールバック1の発火時間のほうが後
 */
static int
timer_handler_cmp(struct _timer_callout *a, struct _timer_callout *b) {

	kassert( (a != NULL) && (b != NULL) );

	if ( a->expire < b->expire )
		return -1;

	if ( a->expire > b->expire )
		return 1;

	return 0;
}

/** ミリ秒をティックカウントに変換
    @param[in] ms ミリ秒での時間
    @return    ティックカウントでの時間
 */
static ticks
ms_to_ticks(tim_tmout ms) {
	
	return ms / (1000 / HZ)  + 1;
}

/** タイマコールバックを初期化する
    @param[in] callout 初期化対象のコールバック情報
 */
static void
init_timer_callout(timer_callout *callout) {
	
	kassert( callout != NULL );

	callout->tmout  = 0;
	callout->expire = 0;
}

/** コールアウトを起動する
    @param[in] cur_tick コールアウト起動時のシステムタイマティック値
 */
void
_tim_invoke_callout(ticks cur_tick) {
	intrflags            flags;
	timer_callout *callout_ref;
	timer_callout        *next;

	spinlock_lock_disable_intr( &timer_callout_queue.lock, &flags );

	for (callout_ref = RB_MIN(timer_queue, &timer_callout_queue.que); callout_ref != NULL; callout_ref = next) {

		next = RB_NEXT(timer_queue, &timer_callout_queue.que, callout_ref);
		if ( callout_ref->expire <= cur_tick ) {

			RB_REMOVE(timer_queue, &timer_callout_queue.que, callout_ref);

			kassert( callout_ref->callout != NULL );
			callout_ref->callout(callout_ref->data);
		}
	}

	spinlock_unlock_restore_intr( &timer_callout_queue.lock, &flags );
	
}

/** タイムアウトに伴うスレッド起床処理
 */
static void
timeout_handler(void *sync) {

	kassert( sync != NULL );

	sync_wake((sync_obj *)sync, SYNC_WAI_TIMEOUT );
}

/** 指定した時間だけCPUを開放する
    @param[in] outms タイムアウト時間(単位:ms)
    @retval SYNC_WAI_TIMEOUT タイムアウトした
 */
sync_reason
tim_wait(tim_tmout outms) {
	intrflags           flags;
	timer_callout          cb;
	sync_obj        timer_obj; 
	sync_reason           res;
	timer_callout        *cbp;

	init_timer_callout( &cb ); /* コールバック情報を初期化  */

	 /*  同期オブジェクトを初期化  */
	sync_init_object( &timer_obj, SYNC_WAKE_FLAG_ALL );

	/*
	 * タイムアウト情報を設定
	 */
	cb.tmout = outms;
	cb.expire = ms_to_ticks(outms) + _tim_refer_uptime_lockfree();
	cb.callout = timeout_handler;
	cb.data = &timer_obj;

	spinlock_lock_disable_intr( &timer_callout_queue.lock, &flags );

	cbp = RB_INSERT(timer_queue, &timer_callout_queue.que, &cb );
	kassert( cbp == NULL );

	spinlock_unlock_restore_intr( &timer_callout_queue.lock, &flags );

	res = sync_wait( &timer_obj );  /*  タイマ待ちに入る  */

	spinlock_lock_disable_intr( &timer_obj.lock, &flags ); 
	kassert( queue_is_empty( &timer_obj.que ) );  /*  キューが空であることを確認  */
	spinlock_unlock_restore_intr( &timer_obj.lock, &flags );

	return res;
}

/** 同期オブジェクトをタイムアウト付きで待ち合わせる
    @param[in] obj   同期オブジェクト
    @param[in] timer_objp タイマ同期オブジェクト
    @param[in] obj_sbp   オブジェクト用同期ブロック
    @param[in] timer_sbp タイマ同期ブロック
    @param[in] cbp       タイマコールバック情報
    @param[in] outms タイムアウト時間(単位:ms)
*/
void
_tim_wait_obj_no_schedule(sync_obj *obj, sync_obj *timer_objp, sync_block *obj_sbp, 
    sync_block *timer_sbp, timer_callout *cbp, tim_tmout outms) {
	intrflags       flags;
	timer_callout    *res;

	kassert( obj != NULL );
	kassert( timer_objp != NULL );
	kassert( timer_sbp != NULL );
	kassert( obj_sbp != NULL );
	kassert( cbp != NULL );

	init_timer_callout( cbp ); /* コールバック情報を初期化  */

	/*  タイムアウト用同期オブジェクトを初期化  */
	sync_init_object( timer_objp, SYNC_WAKE_FLAG_ALL ); 
	_sync_init_block( timer_sbp );  /*  タイムアウト用同期ブロックを初期化  */
	_sync_init_block( obj_sbp );    /*  オブジェクト待ち用同期ブロックを初期化  */

	/*
	 * タイムアウト情報を設定
	 */
	cbp->tmout = outms;
	cbp->expire = ms_to_ticks(outms) + _tim_refer_uptime_lockfree();
	cbp->callout = timeout_handler;
	cbp->data = timer_objp;

	/* キューに入れる前に自スレッドの状態をTHR_TSTATE_WAIT(資源待ち)に設定し, 
	 * タイムアウトや資源開放とのレースコンディションを避ける  
	 */
	current->status = THR_TSTATE_WAIT;  

	spinlock_lock_disable_intr( &timer_callout_queue.lock, &flags ); 
	/*  自スレッドをタイマオブジェクトのキューに登録  */
	queue_add( &timer_objp->que, &timer_sbp->olink); 
        /*  タイマコールバックをタイマキューに追加 */
	res = RB_INSERT(timer_queue, &timer_callout_queue.que, cbp );       
	kassert( res == NULL );
	spinlock_unlock_restore_intr( &timer_callout_queue.lock, &flags );

	spinlock_lock_disable_intr( &obj->lock, &flags );
        /*  自スレッドを同期オブジェクトのキューに追加  */
	queue_add( &obj->que, &obj_sbp->olink);
	spinlock_unlock_restore_intr( &obj->lock, &flags );
}

/** タイムアウト付き同期オブジェクト待ちの起床要因を取り出す
    @param[in] obj   同期オブジェクト
    @param[in] obj_sbp   オブジェクト用同期ブロック
    @param[in] timer_sbp タイマ同期ブロック
    @param[in] cbp       タイマコールバック
    @retval SYNC_WAI_RELEASED  待ち要因が解消された 
    @retval SYNC_OBJ_DESTROYED オブジェクトが破棄された
    @retval SYNC_WAI_TIMEOUT タイムアウトした
*/
sync_reason
_tim_finish_wait_obj(sync_obj *obj, sync_obj *timer_objp, sync_block *obj_sbp, sync_block *timer_sbp, timer_callout *cbp) {
	sync_reason res;
	intrflags flags;	

	/*
	 * イベントが来ている場合は, イベントによる起床を起床要因に仮設定する
	 */
	if ( ev_has_pending_events(current) ) {
		res = SYNC_WAI_DELIVEV;  /*  イベントによる起床  */
	}

	/*
	 * 同期オブジェクトとタイマのキューから自スレッドを削除
	 */
	spinlock_lock_disable_intr( &obj->lock, &flags );   
	if ( obj_sbp->reason != SYNC_WAI_WAIT  ) 
		res = obj_sbp->reason;    /*  待ち解除要因を取得                */
	list_del( &obj_sbp->olink ); /*  同期オブジェクトのキューから削除  */
	spinlock_unlock_restore_intr( &obj->lock, &flags ); 

	spinlock_lock_disable_intr( &timer_objp->lock, &flags );  
	list_del( &timer_sbp->olink ); /* タイマオブジェクトのキューから削除  */
	spinlock_unlock_restore_intr( &timer_objp->lock, &flags );  

	spinlock_lock_disable_intr( &timer_callout_queue.lock, &flags ); 
	if ( timer_sbp->reason == SYNC_WAI_WAIT  ) {
		RB_REMOVE(timer_queue, &timer_callout_queue.que, cbp);
	} else if ( obj_sbp->reason == SYNC_WAI_WAIT )
		res = timer_sbp->reason;   /*  待ち解除要因を更新    */
	spinlock_unlock_restore_intr( &timer_callout_queue.lock, &flags ); 

	/*  タイマオブジェクトが解放されるため, タイマのキューが空であることを確認  */
	spinlock_lock_disable_intr( &timer_objp->lock, &flags );  
	kassert( queue_is_empty( &timer_objp->que ) ); 
	spinlock_unlock_restore_intr( &timer_objp->lock, &flags );

	return res;
}

/** 同期オブジェクトをタイムアウト付きで待ち合わせる
    @param[in] obj   同期オブジェクト
    @param[in] outms タイムアウト時間(単位:ms)
    @retval SYNC_WAI_RELEASED  待ち要因が解消された 
    @retval SYNC_OBJ_DESTROYED オブジェクトが破棄された
    @retval SYNC_WAI_TIMEOUT タイムアウトした
*/
sync_reason
tim_wait_obj(sync_obj *obj, tim_tmout outms) {
	sync_obj   timer_obj;
	sync_block timer_sb;
	sync_block obj_sb;
	timer_callout cb;

	kassert(obj != NULL);

	_tim_wait_obj_no_schedule(obj, &timer_obj, &obj_sb, &timer_sb, &cb, outms);
	sched_schedule();  /*  CPUを解放, 再スケジュールを実施  */
	return _tim_finish_wait_obj(obj, &timer_obj, &obj_sb, &timer_sb, &cb);
}

/** マイクロ秒待ちビジーループ
    @param[in] us ビジーループする時間(単位:マイクロ秒)
 */
void 
udelay(delay_cnt us) {

	hal_udelay(us);
}

/** ミリ秒待ちビジーループ
    @param[in] ms ビジーループする時間(単位:ミリ秒)
 */
void 
mdelay(delay_cnt ms){

	hal_udelay(ms * 1000);
}


/** タイマサブシステムを初期化する
 */
void
tim_init_subsys(void) {

	_tim_setup_uptime_clock();
	hal_timer_init();
	hal_setup_udelay();
}
