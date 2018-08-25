/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Thread reaper routines                                            */
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
#include <kern/kresv-ids.h>

//#define  DEBUG_REAPER_THREAD

static thread_reaper kthread_repaer;

/** 回収処理スレッド
 */
static int
reaper_thread(void __attribute__ ((unused)) *arg) {
	int             rc;
	intrflags    flags;
	thread_reaper *ktr;
	sync_reason    res;
	thread        *thr;
	thread_queue   *tq;

	ktr = &kthread_repaer;
	tq = &ktr->tq;

	while(1) {
				
#if defined(DEBUG_REAPER_THREAD)
		kprintf(KERN_INF, "reaper thread[tid:%d] waiting\n", 
		    current->tid);
#endif  /*  DEBUG_REAPER_THREAD  */

		spinlock_lock_disable_intr( &tq->lock, &flags );

		res = sync_wait( &ktr->wai, &tq->lock );
		kassert( res == SYNC_WAI_RELEASED );

#if defined(DEBUG_REAPER_THREAD)
		kprintf(KERN_INF, "reaper thread[tid:%d] wakeup\n", 
		    current->tid);
#endif  /*  DEBUG_REAPER_THREAD  */

		while( !tq_is_empty( tq ) ) {

			tq_get_top( tq, &thr );
#if defined(DEBUG_REAPER_THREAD)
		kprintf(KERN_INF, "reaper thread[tid:%d] try to destroy:tid=%d[thread=%p. status=%d]\n", 
		    current->tid, thr->tid, thr, thr->status);
#endif  /*  DEBUG_REAPER_THREAD  */
			kassert( thr->status == THR_TSTATE_EXIT );

			rc = thr_destroy(thr);
			kassert( rc == 0 );
		}
		spinlock_unlock_restore_intr( &tq->lock, &flags );
	}

	return 0;
}

/** 自スレッドの回収を依頼する
 */
void
_thr_enter_dead(void) {
	thread_reaper *ktr;
	thread_queue   *tq;
	intrflags    flags;

	kassert( current->status == THR_TSTATE_EXIT );

	ktr = &kthread_repaer;
	tq = &ktr->tq;

#if defined(DEBUG_REAPER_THREAD)
	kprintf(KERN_INF, "thread[tid:%d, thread=%p] enter dead(status=%d).\n", 
	    current->tid, current, current->status);
#endif  /*  DEBUG_REAPER_THREAD  */

	spinlock_lock_disable_intr(&tq->lock, &flags);	
	tq_add( tq, current);  /*  自スレッドを回収対象に追加  */
	spinlock_unlock_restore_intr(&tq->lock, &flags);

	sync_wake(&ktr->wai, SYNC_WAI_RELEASED);  /*  回収処理スレッドを起床  */
}

/** スレッド回収処理の初期化
 */
void
_thr_init_reaper(void) {
	int            rc;
	thread_reaper *ktr;

	ktr = &kthread_repaer;

	rc = thr_new_thread( &ktr->thr );
	kassert( rc == 0 );

	rc = thr_create_kthread( ktr->thr, THR_MAX_PRIO - 1, THR_FLAG_NONE,
	    ID_RESV_REAPER, reaper_thread, (void *)"ThreadReaper" );
	kassert( rc == 0 );
	kassert( ktr->thr->tid == ID_RESV_REAPER );

	sync_init_object( &ktr->wai, SYNC_WAKE_FLAG_ALL, THR_TSTATE_WAIT );
	tq_init( &ktr->tq );

	rc = thr_start(ktr->thr, current->tid);
	kassert( rc == 0 );
}
