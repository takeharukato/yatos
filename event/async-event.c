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
#include <kern/proc.h>
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

/** プロセスにシグナルを送る
    @param[in] p      配送先プロセス
    @param[in] node   シグナルノード
    @retval    0      正常配送完了
    @retval   -ENOENT マスクしていない配送先がおらず自身がマスタスレッドだった
 */
int
ev_send_to_process(proc *p, event_node *node) {
	list        *tli;
	thread      *thr;	
	intrflags  flags;

	kassert( spinlock_locked_by_self( &p->lock ) );

	/*
	 * 配送先スレッドを特定する
	 */
	for( tli = queue_ref_top( &p->threads );
	     tli != (list *)&p->threads;
	     tli = tli->next) {
		
		thr = CONTAINER_OF(tli,thread, plink);
		
		/*  対象イベントをマスクしている場合は除外  */
		if ( ev_mask_test(&thr->evque.masks, node->info.no) )
			continue;  
		
		goto found;  /*  配送先が見つかった  */
	}
		
	thr = current->p->master;  /*  マスターに配送 */

found:
	/*
	 * イベントを配送
	 */
	spinlock_lock_disable_intr( &thr->evque.lock, &flags );
	queue_add( &thr->evque.que[node->info.no], &node->link);  /*  追加  */
	spinlock_unlock_restore_intr( &thr->evque.lock, &flags );

	return 0;
}


/** カレントスレッド終了時の未処理イベントを回送
    @param[in] p 
 */
void
ev_handle_exit_thread_events(void) {
	int            i;
	int           rc;
	list         *li;
	list       *next;
	event_node *node;

	kassert( current->status == THR_TSTATE_EXIT );
	kassert( spinlock_locked_by_self( &current->p->lock ) );

	for( i = 0; EV_NR_EVENT > i ; ++i) {

		for( li = queue_ref_top( &current->evque.que[i]);
		     li != (list *)&current->evque.que[i];
		     li = next) {
			
			next = li->next;
			node = CONTAINER_OF(li, event_node, link);
			list_del( li );

			if ( ( current == current->p->master ) || 
			    ( node->info.flags & EV_FLAGS_THREAD_SPECIFIC ) ) {
				
				/* 自身が最終スレッドだった場合や
				 * スレッド固有イベントの場合は, 
				 * 即時にイベントを破棄する  
				 */
				kfree( node );
				continue;  
			}
			
			/*
			 * イベントを他スレッドに配送
			 */
			rc = ev_send_to_process(current->p, node);
			if ( rc != 0 )
				kfree( node );  /*  配送失敗時は, イベントを破棄  */
		}
	}
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

	return !rc;
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
	int               rc;
	thread          *thr;
	intrflags      flags;

	kassert( node != NULL );

	acquire_all_thread_lock( &flags );

	thr = thr_find_thread_by_tid_nolock(dest);
	if ( thr == NULL ) {

		rc = -ENOENT;
		goto error_out;
	}

	if ( thr->type == THR_TYPE_KERNEL ) {

		rc = -EPERM;
		goto error_out;
	}

	kassert( node->info.no < EV_NR_EVENT);
	kassert( !check_recursive_locked( &thr->evque.lock ) );	

	spinlock_lock( &thr->evque.lock );

	ev_mask_set( &thr->evque.events, node->info.no );
	queue_add( &thr->evque.que[node->info.no], &node->link );
	if ( !ev_mask_test(&thr->evque.masks, node->info.no) ) {
		
		/*  イベントを配送可能な場合はスレッドの待ちを起こす  */
		_sched_wakeup(thr);
	}

	spinlock_unlock( &thr->evque.lock);

	release_all_thread_lock(&flags);
	
	return 0;

error_out:
	release_all_thread_lock(&flags);

	return rc;
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
