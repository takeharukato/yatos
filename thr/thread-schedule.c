/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Thread scheduler routines                                         */
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
#include <kern/sched.h>
#include <kern/idle.h>

static thread *running_threads[NR_CPUS];         /*<  実行中のスレッド  */
static thread_queue ready_queues[THR_MAX_PRIO];  /*<  レディキュー      */

/** レディキューからスレッドを取り出す
    @param[in] prio スレッドの優先度
 */
static thread *
ready_queue_get_top_nolock(thr_prio prio) {
	thread *thr;

	kassert( prio < THR_MAX_PRIO );

	tq_get_top( &ready_queues[prio], &thr );

	return thr;
}

/** レディキューにスレッドを追加する
    @param[in] thr 追加対象のスレッド
 */
static void
ready_queue_add_nolock(thread *thr) {

	kassert( thr != NULL );
	kassert( thr->prio < THR_MAX_PRIO );
	
	tq_add( &ready_queues[thr->prio], thr );
}

/** レディキューにスレッドがいないことを確認する
    @param[in] prio 確認対象の優先度
    @retval true  スレッドキューにスレッドがいない
    @retval false スレッドキューにスレッドがいる
 */
static bool
ready_queue_is_empty_nolock(thr_prio prio) {

	kassert( prio < THR_MAX_PRIO );

	return tq_is_empty( &ready_queues[prio] );
}

/** スレッドを切り替える
    @param[in] thr_prev 切り替え前のスレッド（現在のスレッド)
    @param[in] thr_next 切り替え後のスレッド
 */
static void 
sched_switch_threads(thread *thr_prev, thread *thr_next) {

	if ( thr_prev->p != thr_next->p ) 
		hal_switch_address_space( thr_prev->p,  thr_next->p);

        /* ユーザスレッドの場合, ディスパッチによってユーザ出口処理に移るため,
	 * 本関数に復帰しないため, 切り替え前に状態/例外エントリ用カーネル
	 * スタックを変えないと実行中にならない.
	 */
	thr_next->status = THR_TSTATE_RUN;  
	hal_set_exception_stack( ti_kstack_to_tinfo( thr_next->ksp ) );
	hal_fpu_context_switch(thr_prev, thr_next);
	hal_do_context_switch( &thr_prev->last_ksp, &thr_next->last_ksp );
}

/** 次に実行するスレッドを選択する
    @return 次に実行するスレッドのスレッド構造体
    @note   実行可能なスレッドがいない場合は, アイドルスレッドを返却する
 */
static thread *
ready_queue_get_next_thread(void) {
	int           i;
	intrflags flags;
	thread    *next;

	for( i = ( THR_MAX_PRIO  - 1 ); 0 <= i; --i ) {

		spinlock_lock_disable_intr( &ready_queues[i].lock, &flags);

		if ( !ready_queue_is_empty_nolock( i ) ) {

			next = ready_queue_get_top_nolock( i );
			spinlock_unlock_restore_intr( &ready_queues[i].lock, &flags);
			goto found_next;
		}

		spinlock_unlock_restore_intr( &ready_queues[i].lock, &flags);
	}

	next = idle_refer_idle_thread();

found_next:
	return next;
}
/** スレッドを起床する
    @param[in] 起床対象スレッド
    @note スレッド開始関数/同期機構/非同期イベントを実装するIFのため外部リンケージとして定義
 */
void
_sched_wakeup(thread *thr) {
	intrflags flags;

	kassert( thr != NULL );
	kassert( ( thr->status == THR_TSTATE_WAIT ) || 
	    ( thr->status == THR_TSTATE_READY ) ||
	    ( thr->status == THR_TSTATE_RUN ) ||
	    ( thr->status == THR_TSTATE_DORMANT ) );

	spinlock_lock_disable_intr( &ready_queues[thr->prio].lock, &flags);
	if ( ( thr->status == THR_TSTATE_WAIT ) || ( thr->status == THR_TSTATE_DORMANT ) ) {

		/* 既に起床されたスレッドをキューに入れ直して
		 * キューを破壊しないように, WAIT/DORMANTの場合だけ
		 * レディキューに入れる.
		 */
		thr->status = THR_TSTATE_READY;
		ready_queue_add_nolock( thr );
	}
	ti_set_delay_dispatch(current->ti);  /*  起床に伴うスケジュール要求発行  */
	spinlock_unlock_restore_intr( &ready_queues[thr->prio].lock, &flags);
}

/** スケジューラ本体
 */
void
sched_schedule(void) {
	thread       *next;
	intrflags    flags;
	thread_info    *ti;

	hal_cpu_disable_interrupt(&flags);

	ti = current->ti;

	if ( ti_dispatch_disabled(ti) ) {  /* ディスパッチ禁止区間から呼ばれた  */

		kprintf(KERN_WAR, "sched: scheduler is called in critical section!\n");
		print_back_trace(NULL);
		kassert(!ti_dispatch_disabled(ti));
	}

	ti_clr_thread_info(ti); /* 遅延ディスパッチ要求をクリア  */
	ti->preempt &= ~THR_PREEMPT_ACTIVE;  /*  ディスパッチ要求受付完了  */

	if ( current->status == THR_TSTATE_RUN )
		goto no_need_sched;  /*  スケジューラ呼び出し前に起床された  */

	next = ready_queue_get_next_thread();  /* 次に実行するスレッドを選択  */
	if ( next == current ) /* 他に動作させるスレッドがない  */
		goto no_need_sched;

	if ( ( current != idle_refer_idle_thread() ) &&
	    ( ( current->status == THR_TSTATE_RUN ) || 
		( current->status == THR_TSTATE_READY ) ) ) {
		
		/*
		 * アイドルスレッドを除く実行可能なスレッドをレディキューに戻す
		 */
		spinlock_lock_disable_intr( &ready_queues[current->prio].lock, &flags);
		current->status = THR_TSTATE_READY;
		ready_queue_add_nolock(current);
		spinlock_unlock_restore_intr( &ready_queues[current->prio].lock, &flags);
	}

	sched_switch_threads(current, next);  /*  スレッドの切り替え  */
	kassert( current->status == THR_TSTATE_RUN );  /*  currentは切り替え後のスレッド */

	running_threads[current_cpu()] = current;  /*  カレントCPUの実行中スレッドを更新  */

no_need_sched:
	hal_cpu_restore_interrupt(&flags);
}

/** スケジューラの初期化
    @note レディキューの初期化
 */
void
sched_init_subsys(void) {
	int i;

	/*
	 * レディーキューの初期化
	 */
	for( i = 0; THR_MAX_PRIO > i; ++i ) {
		
		tq_init( &ready_queues[i] );
	}
	
	/*
	 * ランニングスレッド表の初期化
	 */
	for( i = 0; NR_CPUS > i; ++i) {

		running_threads[i] = NULL;
	}
}
