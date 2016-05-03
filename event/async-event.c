/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Asynchronous event relevant routines                              */
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
#include <kern/async-event.h>
#include <kern/queue.h>
#include <kern/list.h>
#include <kern/page.h>

/** イベントキューを初期化する
    @param[in] que 初期化対象のキュー
 */
void
ev_queue_init(event_queue *que){
	int i;

	kassert( que != NULL );

	spinlock_init( &que->lock );

	for( i = 0; EV_MAP_LEN > i ; ++i) {
		
		que->events.map[i] = 0;
		que->masks.map[i] = 0;
	}

	for( i = 0; EV_NR_EVENT > i ; ++i) 
		queue_init( &que->que[i] );
}
/** 未配送のイベントを開放する
    @param[in] que 操作対象のキュー
 */
void
ev_free_pending_events(event_queue *que){
	int            i;
	intrflags  flags;
	list         *li;
	list       *next;
	event_node *node;

	kassert( que != NULL );

	spinlock_lock_disable_intr( &que->lock, &flags );
	for( i = 0; EV_NR_EVENT > i ; ++i) {

		for( li = queue_ref_top(&que->que[i]);
		     li != (list *)&que->que[i];
		     li = next) {

			next = li->next;

			list_del(li);
			node =CONTAINER_OF(li , event_node, link);
			kfree(node);
		}
	}
	spinlock_unlock_restore_intr( &que->lock, &flags );
}

/** イベントマスクを取得する
    @param[in] mask 取得したマスクの返却先
 */
void
ev_get_mask(event_mask *mask) {
	intrflags flags;

	kassert( mask != NULL );

	spinlock_lock_disable_intr( &current->evque.lock, &flags );
	memcpy(mask, &current->evque.masks, sizeof( event_mask ) );
	spinlock_unlock_restore_intr( &current->evque.lock, &flags );	
}

/** 自スレッドのマスクを設定する
    @param[in] mask 設定するマスク
 */
void
ev_update_mask(event_mask *mask){
	intrflags flags;

	kassert( mask != NULL );

	spinlock_lock_disable_intr( &current->evque.lock, &flags );
	memcpy(&current->evque.masks, mask, sizeof( event_mask ) );
	spinlock_unlock_restore_intr( &current->evque.lock, &flags );	
}

/** 未配送のイベントがあることを確認する
    @param[in] thr 確認対象のスレッド
    @retval    真  未配送のイベントがある
    @retval    偽  未配送のイベントがない
 */
bool
ev_has_pending_events(thread *thr){
	bool         rc;
	intrflags flags;

	kassert( thr != NULL );

	spinlock_lock_disable_intr( &thr->evque.lock, &flags );
	rc = ev_mask_empty( &thr->evque.events );
	spinlock_unlock_restore_intr( &thr->evque.lock, &flags );

	return rc;
}
/** イベントを送信する
    @param[in] dest  送信先スレッド
    @param[in] node  イベント情報のノード
    @retval  0       送信完了
    @retval -ENOENT  送信先スレッドが見つからなかった
    @retval -EPERM   カーネルスレッドにイベントを配送しようとした
 */
int
ev_send(tid dest, event_node *node) {
	thread          *thr;
	intrflags      flags;

	kassert( node != NULL );

	thr = thr_find_thread_by_tid(dest);
	if ( thr == NULL )
		return -ENOENT;

	if ( thr->type == THR_TYPE_KERNEL )
		return -EPERM;

	kassert( node->info.no < EV_NR_EVENT);
	kassert( !check_recursive_locked( &thr->evque.lock ) );	

	spinlock_lock_disable_intr( &thr->evque.lock, &flags);

	ev_mask_set( &thr->evque.events, node->info.no );
	queue_add( &thr->evque.que[node->info.no], &node->link );
	if ( !ev_mask_test(&thr->evque.masks, node->info.no) ) {
		
		/*  イベントを配送可能な場合はスレッドの待ちを起こす  */
		_sched_wakeup(thr);
	}

	spinlock_unlock_restore_intr( &thr->evque.lock, &flags );	
	
	return 0;
}

/** イベントを取り出す
    @param[in] nodep   イベントノードのアドレスを格納する領域
    @retval    0       取得完了
    @retval   -ENOENT  イベントキューが空だった
 */
int
ev_dequeue(event_node **nodep) {
	int               rc;
	intrflags      flags;
	event_mask       tmp;
	event_mask   deliver;
	event_id          id;

	kassert( nodep != NULL);

	spinlock_lock_disable_intr( &current->evque.lock, &flags );
	
	/*
	 * 送信可能な（マスクされていない)イベントの番号を取得する
	 */
	ev_mask_clr( &tmp );
	ev_mask_clr( &deliver );
	
	ev_mask_xor(&current->evque.events, &current->evque.masks, &tmp);
	ev_mask_and(&current->evque.events, &tmp, &deliver);
	
	rc = ev_mask_find_first_bit(&deliver, &id);
	if ( rc != 0 )
		goto unlock_out;
	
	kassert( id < EV_NR_EVENT );
	kassert( !queue_is_empty( &current->evque.que[id] ) );
	
	*nodep = CONTAINER_OF(queue_get_top( &current->evque.que[id] ), 
	    event_node, link);
	
	if ( queue_is_empty( &current->evque.que[id] ) ) 
		ev_mask_unset(&current->evque.events, id);

unlock_out:
	spinlock_unlock_restore_intr( &current->evque.lock, &flags );	
	return rc;
}

/** 捕捉不能イベントを処理する
    @param[in] node    イベントノード
    @retval    0       処理完了
    @retval   -ENOENT  捕捉不能イベントではない  
 */
int
kcom_handle_system_event(event_node *node) {
	evinfo *info;

	kassert( node != NULL );

	info = &node->info;

	if  ( info->no != EV_SIG_KILL )
		return -ENOENT;

	thr_exit(info->code);   /*  自スレッド終了  */

	return 0;
}
