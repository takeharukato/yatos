/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*       routines                                                     */
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

#if defined(CONFIG_SMP)
extern uint32_t x86_64_xchg(volatile uint32_t *_addr, uint32_t _newval);
/** スピンロックの実装部
    @param[in] lock 獲得対象のスピンロック
 */
void 
hal_spinlock_lock(spinlock *lock) {
	
	while(x86_64_xchg(&lock->locked, 1) != 0);
}

/** スピンアンロックの実装部
    @param[in] lock 解放対象のスピンロック
 */
void 
hal_spinlock_unlock(spinlock *lock) {

	x86_64_xchg(&lock->locked, 0);
}

#else  /*  !CONFIG_SMP  */
/** スピンロックの実装部(ユニプロセッサ版)
    @param[in] lock 獲得対象のスピンロック
 */
void 
hal_spinlock_lock(spinlock __attribute__ ((unused)) *lock) {

}

/** スピンアンロックの実装部(ユニプロセッサ版)
    @param[in] lock 解放対象のスピンロック
 */
void 
hal_spinlock_unlock(spinlock __attribute__ ((unused)) *lock) {

}
#endif  /*  CONFIG_SMP  */
