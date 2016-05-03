/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  CPU level inerrupt routines                                       */
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

#include <hal/arch-cpu.h>

extern void x86_64_cpu_disable_interrupt(uint64_t *flags);
extern void x86_64_cpu_restore_interrupt(uint64_t *flags);
extern void x86_64_cpu_enable_interrupt(void);
extern void x86_64_cpu_save_flags(intrflags *flags);

/** CPUへの割込み禁止状態を保存した後, CPUへの割込みを禁止する
    @param[in] flags 割込禁止状態の保存先アドレス
 */
void
hal_cpu_disable_interrupt(intrflags *flags){

	x86_64_cpu_disable_interrupt(flags);
}

/** CPUへの割込み禁止状態を復元する
    @param[in] flags 割込禁止状態の保存先アドレス
 */
void
hal_cpu_restore_interrupt(intrflags *flags){

	x86_64_cpu_restore_interrupt(flags);
}

/** CPUへの割込みを許可する
 */
void
hal_cpu_enable_interrupt(void) {

	x86_64_cpu_enable_interrupt();
}

/** CPUへの割込みが禁止されていることを確認する
    (HAL無効化時のエミュレーション)
    @retval true  CPUへの割込みが禁止されている
    @retval false CPUへの割込みが禁止されていない
 */
bool 
hal_cpu_interrupt_disabled(void){
	intrflags flags;

	x86_64_cpu_save_flags(&flags);

	return 	!( flags & X86_64_RFLAGS_IF );
}
