/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Thread info relevant routines                                     */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/spinlock.h>
#include <kern/id-bitmap.h>
#include <kern/thread.h>
#include <kern/sched.h>

/** スレッド情報を設定する
    @param[in] thr 関連づけるスレッド構造体
    @note スレッド管理モジュール間で使用するため外部リンケージ
    として定義
 */
void
_ti_set_ti_with_thread(thread *thr) {
	thread_info *tinfo;

	kassert( thr != NULL );
	kassert( thr->ksp != NULL );

	tinfo = ti_kstack_to_tinfo( thr->ksp );

	tinfo->magic = THR_THREAD_INFO_MAGIC;
	tinfo->intrcnt = 0;
	tinfo->preempt = 0;
	tinfo->flags = 0;
	tinfo->thr = thr;
	tinfo->cpu = current_cpu();
	hal_ti_init(tinfo);
}

/** ディスパッチ許可状態に設定する
    @param[in] tinfo スレッド情報のアドレス
 */
void
ti_enable_dispatch(void) {
	thread_info *tinfo = ti_get_current_tinfo();
	intrflags flags;

	kassert(tinfo != NULL);

	hal_cpu_disable_interrupt(&flags);

	kassert(tinfo->preempt > 0);
	--tinfo->preempt;

	if ( THR_PREEMPT_ACTIVE & tinfo->preempt )
		goto error_out;

	if ( !( tinfo->flags & THR_DISPATCH_DELAYED ) )
		goto error_out;

	if  ( ( tinfo->preempt != 0 ) || ( tinfo->intrcnt != 0 ) )
		goto error_out;

	tinfo->preempt |= THR_PREEMPT_ACTIVE;
	hal_cpu_restore_interrupt(&flags);
	sched_schedule();
	hal_cpu_disable_interrupt(&flags);

error_out:
	hal_cpu_restore_interrupt(&flags);
}

/** ディスパッチ禁止状態に設定する
 */
void
ti_disable_dispatch(void) {
	thread_info *tinfo = ti_get_current_tinfo();
	intrflags flags;

	kassert(tinfo != NULL);
	kassert( ( THR_PREEMPT_ACTIVE - 1 ) >= (tinfo->preempt & ~THR_PREEMPT_ACTIVE));

	hal_cpu_disable_interrupt(&flags);

	++tinfo->preempt;

	if ( THR_PREEMPT_ACTIVE & tinfo->preempt )
		goto error_out;

error_out:
	hal_cpu_restore_interrupt(&flags);
}

/** ディスパッチ不可能であることを確認する
    @param[in] tinfo スレッド情報のアドレス
 */
int 
ti_dispatch_disabled(thread_info *tinfo) {

	kassert(tinfo != NULL);

	return ( ( ( ~THR_PREEMPT_ACTIVE & tinfo->preempt ) != 0 ) ||
	    ( tinfo->intrcnt > 0 ) );
}

/** 遅延ディスパッチ予約を立てる
    @param[in] tinfo スレッド情報のアドレス
 */
void
ti_set_delay_dispatch(thread_info *tinfo) {

	kassert(tinfo != NULL);

	tinfo->flags |= THR_DISPATCH_DELAYED;
}

/** 遅延ディスパッチ予約をクリアする
    @param[in] tinfo スレッド情報のアドレス
 */
void
ti_clr_delay_dispatch(thread_info *tinfo) {

	kassert(tinfo != NULL);

	tinfo->flags &= ~THR_DISPATCH_DELAYED;
}

/** ディスパッチ予約中であることを確認する
    @param[in] tinfo スレッド情報のアドレス
 */
int 
ti_dispatch_delayed(thread_info *tinfo) {
	
	kassert(tinfo != NULL);

	return ( tinfo->flags & THR_DISPATCH_DELAYED );
}

/** イベント予約を立てる
    @param[in] tinfo スレッド情報のアドレス
 */
void
ti_set_event(thread_info *tinfo) {

	kassert(tinfo != NULL);

	tinfo->flags |= THR_EVENT_PENDING;
}

/** イベント予約ををクリアする
    @param[in] tinfo スレッド情報のアドレス
 */
void
ti_clr_event(thread_info *tinfo) {

	kassert(tinfo != NULL);

	tinfo->flags &= ~THR_EVENT_PENDING;
}

/** イベント予約中であることを確認する
    @param[in] tinfo スレッド情報のアドレス
 */
int 
ti_event_pending(thread_info *tinfo) {
	
	kassert(tinfo != NULL);

	return ( tinfo->flags & THR_EVENT_PENDING  );
}

/** プリエンプション関連フラグをクリアする
    @param[in] tinfo スレッド情報のアドレス
 */
void
ti_clr_thread_info(thread_info *tinfo) {
	
	kassert(tinfo != NULL);

	tinfo->preempt = 0;
	ti_clr_delay_dispatch(tinfo);
}

/** 割り込み多重度をインクリメントする
 */
void
ti_inc_intr(void) {
	intrflags   flags;
	thread_info *tinfo;
	
	hal_cpu_disable_interrupt(&flags);

	tinfo = ti_get_current_tinfo();
	++tinfo->intrcnt;
	hal_cpu_restore_interrupt(&flags);
}

/** 割り込み多重度をデクリメントする
 */
void
ti_dec_intr(void) {
	intrflags   flags;
	thread_info *tinfo;
	
	hal_cpu_disable_interrupt(&flags);

	tinfo = ti_get_current_tinfo();

	kassert( tinfo->intrcnt > 0 );
	--tinfo->intrcnt;

	hal_cpu_restore_interrupt(&flags);
}

/** 割り込み処理中の場合真を返す
 */
bool
ti_in_intr(void) {
	bool            rc;
	intrflags    flags;
	thread_info *tinfo;

	hal_cpu_disable_interrupt(&flags);

	tinfo = ti_get_current_tinfo();
	rc = ( tinfo->intrcnt != 0 );

	hal_cpu_restore_interrupt(&flags);

	return rc;
}

/** カーネルスタックの先頭アドレスを取得する
 */
void *
ti_get_current_kstack_top(void) {
	uintptr_t stack_top, sp;

	get_stack_pointer(&sp);
	stack_top = ( ( (uintptr_t)sp ) & ~( (uintptr_t)(KSTACK_SIZE - 1) ) );

	return (void *)stack_top;
}

/** スタックの先頭アドレスからスレッド情報を取得する
 */
thread_info *
ti_kstack_to_tinfo(void *kstack_top) {

	return (thread_info *)
		( ( (uintptr_t)kstack_top ) + KSTACK_SIZE - THREAD_INFO_SIZE );
}

/** カレントスレッドのスレッド情報を取得する
 */
thread_info *
ti_get_current_tinfo(void) {

	return ti_kstack_to_tinfo( ti_get_current_kstack_top() );
}

/** カレントスレッドのスレッド構造体を取得する
    @note currentマクロ実装用の関数, カーネル内では, currentを使用。
 */
struct _thread *
ti_get_current_thread(void) {

	return ( (thread_info *)ti_get_current_tinfo() )->thr;
}

