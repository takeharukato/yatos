/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Backtrace routines                                                */
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
#include <kern/backtrace.h>
#include <kern/thread.h>

/** カーネルコンソールにバックトレース情報を表示する
    @param[in]  depth   表示対象フレームの段数
    @param[in]  bpref   ベースポインタのアドレス
    @param[in]  caller  呼出し元アドレス
    @param[in]  next_bp 前のフレームのベースポインタ
    @param[in]  argp   _trace_kconsoleの呼び出し元から引き渡されるプライベートデータ
    @retval     0      正常に表示した。
    @retval     負     最大深度に達した
 */
static int
_trace_kconsole(int depth, uintptr_t *bpref, void *caller, void *next_bp, 
    void __attribute__((__unused__)) *argp){

	if (depth >= BACKTRACE_MAX_DEPTH )
		return -1;

	kprintf(KERN_DBG, "[%d] Caller:%p, bp:%p next-bp:%p\n", 
	    depth, caller, bpref, next_bp);

	return 0;
}


/** 指定されたベースポインタからのバックトレース情報を表示する
    @param[in] basep ベースポインタの値(NULLの場合は, 現在のスタックから算出 )
 */
void
print_back_trace(void *basep) {

	if ( ( basep != NULL) && ( ( basep < (void *)ti_get_current_kstack_top() ) ||
		( basep > ( (void *)ti_get_current_kstack_top() + KSTACK_SIZE ) ) ) )
		kprintf(KERN_WAR, "%rbp does not point valid stack:%p (kstack-range:[%p - %p)\n",
		    basep, (void *)ti_get_current_kstack_top(), 
		    ( (void *)ti_get_current_kstack_top() + KSTACK_SIZE ) );

	hal_back_trace(_trace_kconsole, basep, NULL);
}
