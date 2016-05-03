/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  spinlock routines                                                 */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>

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

static int
_trace_spinlock(int depth, uintptr_t __attribute__((__unused__)) *bpref, void *caller, 
		void __attribute__((__unused__)) *next_bp, void  *argp){
	spinlock *lock;

	if ( ( argp == NULL ) || ( depth >= KERN_SPINLOCK_BT_DEPTH) )
		return -1;
	
	lock = (spinlock *)argp;
	lock->backtrace[depth] = (uintptr_t)caller;

	return 0;
}

static void
fill_spinlock_trace(spinlock *lock) {

	memset(&lock->backtrace[0], 0, sizeof(uintptr_t) * KERN_SPINLOCK_BT_DEPTH);
	hal_back_trace(_trace_spinlock, NULL, (void *)lock);
}


#if defined(CONFIG_SMP)

/** ロックを初期化する
    @param[in] lock 初期化対象のスピンロック
 */
void 
spinlock_init(spinlock *lock){

	lock->locked = 0;
	lock->cpu = 0;
}

/** スピンロックを獲得する
    @param[in] lock 獲得対象のスピンロック    
 */
void 
spinlock_lock(spinlock *lock) {

	ti_disable_dispatch();
	hal_spinlock_lock(lock);
	fill_spinlock_trace(lock);
}

/** スピンロックを解放する
    @param[in] lock 解放対象のスピンロック    
 */
void
spinlock_unlock(spinlock *lock) {

	hal_spinlock_unlock(lock);
	ti_enable_dispatch();
}

/** プリエンプションに影響せず割込禁止付きでスピンロックを獲得する
    @param[in] lock 獲得対象のスピンロック    
    @param[in] flags 割込状態保存先アドレス
 */
void
raw_spinlock_lock_disable_intr(spinlock  *lock, intrflags *flags) {

	hal_cpu_disable_interrupt(flags);
	hal_spinlock_lock(lock);
}

/** プリエンプションに影響せずスピンロックを解放し, 割込状態を元に戻す
    @param[in] lock 獲得対象のスピンロック    
    @param[in] flags 割込状態保存先アドレス
 */
void
raw_spinlock_unlock_restore_intr(spinlock *lock, intrflags *flags) {

	hal_spinlock_unlock(lock);
	hal_cpu_restore_interrupt(flags);
}

/** 割込禁止付きでスピンロックを獲得する
    @param[in] lock 獲得対象のスピンロック    
    @param[in] flags 割込状態保存先アドレス
 */
void
spinlock_lock_disable_intr(spinlock  *lock, intrflags *flags) {

	hal_cpu_disable_interrupt(flags);
	ti_disable_dispatch();
	hal_spinlock_lock(lock);
	fill_spinlock_trace(lock);
}

/** スピンロックを解放し, 割込状態を元に戻す
    @param[in] lock 獲得対象のスピンロック    
    @param[in] flags 割込状態保存先アドレス
 */
void
spinlock_unlock_restore_intr(spinlock *lock, intrflags *flags) {

	hal_spinlock_unlock(lock);
	ti_enable_dispatch();
	hal_cpu_restore_interrupt(flags);
}

/** スピンロックを自スレッドが保持していることを確認する
    @param[in] lock 確認対象のスピンロック    
    @retval true  ロックを自分自身で獲得している    
    @retval false ロックを自分自身で獲得していない
 */
bool 
spinlock_locked_by_self(spinlock *lock) {

	return ( ( lock->locked != 0 ) && ( lock->cpu == current_cpu() ) ); 
}

/** スピンロックを多重に獲得していることを確認する
    @param[in] lock 確認対象のスピンロック
    @retval true  ロックを自分自身で獲得している    
    @retval false ロックを自分自身で獲得していない
 */
bool
check_recursive_locked(spinlock *lock) {

	return spinlock_locked_by_self(lock); 
}

#else  /*  !CONFIG_SMP  */

/** ロックを初期化する(ユニプロセッサ版)
    @param[in] lock 初期化対象のスピンロック
 */
void 
spinlock_init(spinlock *lock){

	lock->locked = 0;
	lock->cpu = 0;
}

/** スピンロックを獲得する(ユニプロセッサ版)
    @param[in] lock 獲得対象のスピンロック    
 */
void 
spinlock_lock(spinlock __attribute__ ((unused)) *lock) {

	ti_disable_dispatch();
	fill_spinlock_trace(lock);
}

/** スピンロックを解放する(ユニプロセッサ版)
    @param[in] lock 解放対象のスピンロック    
 */
void
spinlock_unlock(spinlock __attribute__ ((unused)) *lock) {

	ti_enable_dispatch();
}

/** プリエンプションに影響せず割込禁止付きでスピンロックを獲得する
    (ユニプロセッサ版)
    @param[in] lock 獲得対象のスピンロック    
    @param[in] flags 割込状態保存先アドレス
 */
void
raw_spinlock_lock_disable_intr(spinlock __attribute__ ((unused))  *lock, 
    intrflags *flags) {

	hal_cpu_disable_interrupt(flags);	
}

/** プリエンプションに影響せずスピンロックを解放し, 割込状態を元に戻す
    (ユニプロセッサ版)
    @param[in] lock 獲得対象のスピンロック    
    @param[in] flags 割込状態保存先アドレス
 */
void
raw_spinlock_unlock_restore_intr(spinlock __attribute__ ((unused)) *lock, 
    intrflags *flags) {

	hal_cpu_restore_interrupt(flags);
}

/** 割込禁止付きでスピンロックを獲得する(ユニプロセッサ版)
    @param[in] lock 獲得対象のスピンロック    
    @param[in] flags 割込状態保存先アドレス
 */
void
spinlock_lock_disable_intr(spinlock *lock, 
    intrflags *flags) {

	hal_cpu_disable_interrupt(flags);	
	ti_disable_dispatch();
	fill_spinlock_trace(lock);
}

/** スピンロックを解放し, 割込状態を元に戻す(ユニプロセッサ版)
    @param[in] lock 獲得対象のスピンロック    
    @param[in] flags 割込状態保存先アドレス
 */
void
spinlock_unlock_restore_intr(spinlock __attribute__ ((unused)) *lock, 
    intrflags *flags) {

	ti_enable_dispatch();
	hal_cpu_restore_interrupt(flags);
}

/** スピンロックを自スレッドが保持していることを確認する(ユニプロセッサ版)
    @param[in] lock 確認対象のスピンロック    
    @retval TRUE  ロックを自分自身で獲得している    
    @retval FALSE ロックを自分自身で獲得していない
 */
bool
spinlock_locked_by_self(spinlock __attribute__ ((unused)) *lock) {

	return true;   /* Uni Processorでは常に真  */
}

/** スピンロックを多重に獲得していることを確認する(ユニプロセッサ版)
    @param[in] lock 獲得対象のスピンロック    
    @retval TRUE  ロックを自分自身で獲得している    
    @retval FALSE ロックを自分自身で獲得していない
 */
bool
check_recursive_locked(spinlock __attribute__ ((unused)) *lock) {

	return false;  /* Uni Processorでは自身のロックでデッドロックしない  */
}
#endif  /*  CONFIG_SMP  */
