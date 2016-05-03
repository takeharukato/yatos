/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Idle thread routines                                              */
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
#include <kern/sched.h>
#include <kern/idle.h>

#include <thr/thr-internal.h>

static thread idle_threads[NR_CPUS];

/** BSP/AP共通のアイドルループ処理
 */
void
_thr_do_idle(void) {
	intrflags flags;

	kassert( hal_cpu_interrupt_disabled() );

	while( 1 ) {

		hal_cpu_disable_interrupt(&flags);

		thr_yield();  /*  CPUを解放  */

                /* ディスパッチ要求がなければ,
		 * アーキ依存のアイドル処理を実施
		 * (例: 割込発生までCPUを停止)。
		 */
		if ( !ti_dispatch_delayed(ti_get_current_tinfo()) ) 
			hal_idle(); 

		hal_cpu_restore_interrupt(&flags);
	}
}

/** 自CPUのアイドルスレッドを返却する
 */
struct _thread *
idle_refer_idle_thread(void) {

	return &idle_threads[current_cpu()];
}

/** 自CPUのアイドルスレッドを初期化する
 */
void
idle_init_current_cpu_idle(void) {
	thread *thr;

	thr = &idle_threads[current_cpu()];

	thr->type = THR_TYPE_KERNEL;       /*  無属性スレッドに設定  */
	thr->ti = ti_get_current_tinfo();  /*  スレッド情報の設定  */
	thr->ti->thr = thr;                /*  スレッド情報からのスレッド参照の設定  */
	ti_clr_thread_info( thr->ti );     /*  ディスパッチ制御情報の初期化  */
}

/** BSPのアイドルループを開始
 */
void
idle_start(void) {

	_thr_do_idle();
}

/** アイドルスレッド機能の初期化
 */
void
idle_init_subsys(void) {
	int i;
	thread *thr;

	for( i = 0; NR_CPUS > i; ++i ) {

		thr = &idle_threads[i];
		_thr_init_kthread_params( thr );
		thr->tid = THR_IDLE_TID;
	}
	idle_init_current_cpu_idle();  /*  BSPのアイドルスレッドを初期化する */
	kassert( current->p == hal_refer_kernel_proc() );
	current->p->master = current;  /*  カーネルプロセス空間のマスタースレッドに設定 */
}
