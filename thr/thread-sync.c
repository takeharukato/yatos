/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Thread synchronization relevant definitions                       */
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
#include <kern/list.h>
#include <kern/thread.h>
#include <kern/sched.h>

#include <thr/thr-internal.h>

/** 同期ブロックを初期化し, 自スレッドを待ちスレッドに設定する
    @param[in] blk 初期化対象の同期ブロック
*/
void
_sync_init_block(sync_block *blk){
	
	kassert(blk != NULL);
	
	blk->thr = current;
	list_init( &blk->olink );
	blk->reason = SYNC_WAI_WAIT;
}

/** 同期オブジェクトを初期化する
    @param[in] obj    初期化対象の同期オブジェクト
    @param[in] policy スレッド起床方針
*/
void
sync_init_object(sync_obj *obj, sync_pol pol) {
	
	kassert(obj != NULL);

	spinlock_init( &obj->lock );
	obj->policy = pol;
	queue_init( &obj->que );
}

/** 同期オブジェクトを待ち合わせる
    @param[in] obj 同期オブジェクト
    @param[in] blk 同期ブロック
*/
void
_sync_wait_no_schedule(sync_obj *obj, sync_block *blkp) {
	intrflags flags;

	kassert( obj != NULL );
	kassert( blkp != NULL );

	_sync_init_block( blkp );  /*  同期ブロックを初期化  */

	spinlock_lock_disable_intr( &obj->lock, &flags );   /* 同期オブジェクトのロックを獲得 */

	current->status = THR_TSTATE_WAIT;        /* 自スレッドの状態をTHR_TSTATE_WAIT(資源待ち)に設定  */

	queue_add( &obj->que, &blkp->olink );  /*  blkを同期オブジェクトのキューに追加  */
	
	spinlock_unlock_restore_intr( &obj->lock, &flags );  /* 同期オブジェクトのロックを解放 */

	return;
}
/** 同期待ちの後処理を実施
    @param[in] obj 同期オブジェクト
    @param[in] blk 同期ブロック
    @retval SYNC_WAI_RELEASED  待ち要因が解消された 
    @retval SYNC_OBJ_DESTROYED オブジェクトが破棄された
 */
sync_reason
_sync_finish_wait(sync_obj *obj, sync_block *blk) {
	intrflags flags;
	sync_reason res;

	kassert( obj != NULL );
	kassert( blk != NULL );

	res = blk->reason;
	spinlock_lock_disable_intr( &obj->lock, &flags ); 
	if ( ( ev_has_pending_events(current) ) &&
	    ( !list_not_linked( &blk->olink ) ) ) {
		
		list_del( &blk->olink );
		res = SYNC_WAI_DELIVEV;  /*  イベントによる起床  */
	}
	spinlock_unlock_restore_intr( &obj->lock, &flags ); 

	return res;
}

/** 同期オブジェクトを待ち合わせる
    @param[in] obj 同期オブジェクト
    @retval SYNC_WAI_RELEASED  待ち要因が解消された 
    @retval SYNC_OBJ_DESTROYED オブジェクトが破棄された
*/
sync_reason
sync_wait(sync_obj *obj) {
	sync_block blk;
	sync_reason rc;

	_sync_wait_no_schedule(obj, &blk);
	sched_schedule();  /*  CPUを解放, 再スケジュールを実施  */
	rc = _sync_finish_wait(obj, &blk);

	return rc;  /*  起床要因を返却  */
}

/** 同期オブジェクトを待ち合わせる
    @param[in] obj 同期オブジェクト
    @param[in] blk 同期ブロック
*/
void
sync_wake(sync_obj *obj, sync_reason reason) {
	intrflags flags;
	sync_block *blk;

	kassert(obj != NULL);

	spinlock_lock_disable_intr( &obj->lock, &flags );  

	while( !queue_is_empty( &obj->que ) ) {
		
		blk = CONTAINER_OF( queue_get_top( &obj->que ),
		    sync_block, olink);  /*  キューの先頭の同期ブロックを取り出し  */

		/*  同期ブロックのreasonに引数reasonで指定された値をセット  */		
		blk->reason = reason; 

		_sched_wakeup( blk->thr );  /*  同期ブロックが指し示すスレッドを起床  */
		
		/*
		 * 同期オブジェトのスレッド起床方針が
		 * SYNC_WAKE_FLAG_ONEだった場合は, ループから抜ける
		 */
		if ( obj->policy == SYNC_WAKE_FLAG_ONE )
			break;
	}
	
	spinlock_unlock_restore_intr( &obj->lock, &flags );
	/*  スレッド起床に伴う再スケジュール  */
	if ( !ti_dispatch_disabled(current->ti) ) 	 
		sched_schedule();
}
