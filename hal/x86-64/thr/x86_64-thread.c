/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*   Architecture dependent thread relevant routines                  */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/spinlock.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/thread.h>
#include <kern/page.h>

#include <hal/kernlayout.h>
#include <hal/userlayout.h>
#include <hal/arch-cpu.h>
#include <hal/segment.h>
#include <hal/traps.h>

#define USER_CTX_MAGIC (0xfeed1234)

extern void x86_64_start_kthread(void);
extern void x86_64_retrun_to_user(void);

/** アーキ固有のスレッド情報初期化
    @param[in] tinfo 初期化対象のスレッド情報
 */
void
hal_ti_init(thread_info *tinfo){
	
	kassert( tinfo != NULL );
	tinfo->arch_flags = 0;
}
/** スレッドスタックにカーネルスレッド開始関数情報をセットする
    @param[in] thr  スレッド管理情報
    @param[in] fn   スレッドの開始関数
    (カーネル内プロセス出口処理またはカーネルスレッド開始エントリ)
    @param[in] arg  スレッドの開始関数の引数
 */
void
hal_setup_kthread_function(struct _thread *thr, int (*fn)(void *), void *arg) {
	uintptr_t *sp;

	kassert( thr != NULL );
	kassert( thr->type == THR_TYPE_KERNEL );
	kassert( fn != NULL );
	kassert( (uintptr_t)fn >= KERN_VMA_BASE );

        /*
	 * スレッド管理情報(カーネルスレッド)の
	 * 一つ上の段から引数, スレッド開始アドレス, 
	 * 自スレッド終了処理関数(スレッド開始アドレスからの復帰アドレス)
	 * を積み上げる.
	 */
	sp = (uintptr_t *)thr->last_ksp;
	--sp;  

	*sp-- = (uintptr_t)arg;
	*sp-- = (uintptr_t)fn;
	/* thr_exit に戻るコンテキストを念のため積んでおく  */
	*sp-- = (uintptr_t)thr_exit; 
	/* thread_start関数呼び出し用のグルー */
	*sp = (uintptr_t)x86_64_start_kthread; 

	thr->last_ksp = sp;   /* スタックポインタを更新する  */
}

/** ユーザスレッド用のスタックを用意する
    @param[in] start  ユーザランド開始アドレス
    @param[in] arg1      ユーザランドに引き渡す第1引数(argc)
    @param[in] arg2      ユーザランドに引き渡す第2引数(argv)
    @param[in] arg3      ユーザランドに引き渡す第3引数(environment)
    @param[in] usp    ユーザスタックの開始アドレス
    @param[in] kstack カーネルスタックの先頭アドレス
    @param[in] spp スタックの現在位置返却先アドレス
    @retval   0 正常終了
 */
int
hal_setup_uthread_kstack(void *start, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
    void *usp, void *kstack, void **spp){
	uintptr_t     *sp;
	trap_context *ctx;

	kassert(kstack != NULL);
	kassert(spp != NULL);

	/* スレッド情報の上に例外コンテキストを積み上げる  */
	ctx = (trap_context *)( (uintptr_t)ti_kstack_to_tinfo(kstack)
	    - sizeof(trap_context) );

	/*
	 * ユーザ空間に復帰するためのコンテキストを積込み
	 */
	ctx->rip = (uint64_t)start; 
	ctx->cs = GDT_USER_CODE64;
	ctx->rflags = ( (uint64_t)1 << X86_64_RFLAGS_IF );  /*  割込み許可フラグのみ立てる */
	ctx->rsp = ( ( usp == NULL) ? ( (uint64_t)USER_STACK_BOTTOM ) : ( (uint64_t)usp ) );
	ctx->ss = GDT_USER_DATA64;
	ctx->rdi = (uint64_t)arg1;
	ctx->rsi = (uint64_t)arg2;
	ctx->rdx = (uint64_t)arg3;
	ctx->rax = USER_CTX_MAGIC;

	/*
	 * コンテキストの一つ上の段からスレッド開始アドレスとして
	 * ユーザ例外出口処理 を積み上げる.
	 */
	sp = ( ( (uintptr_t *)ctx ) - 1 );
	*sp = (uintptr_t)x86_64_retrun_to_user; 

	*spp = sp;

	return 0;
}
/** スレッドのFPUコンテキストを初期化する
 */
void
hal_fpctx_init(fpu_context *fpctx) {

	kassert( fpctx != NULL );

	memset(fpctx, 0, sizeof(fpu_context) );
}

/** スレッドのFPUコンテキストを切り替える
    @param[in] thr_prev CPUを解放するスレッド
    @param[in] thr_next CPUを獲得するスレッド(未使用)
 */
void
hal_fpu_context_switch(thread *thr_prev, thread __attribute__ ((unused)) *thr_next) {
	thread_info *ti;

	kassert( thr_prev != NULL );

	ti = thr_prev->ti;

	if ( ti->arch_flags & TI_X86_64_FPU_USED ) {  

                /*
		 * FPUを使用していた場合はFPUコンテキストを保存する  
		 */
		x86_64_fxsave( &thr_prev->fpctx );
		ti->arch_flags &= ~TI_X86_64_FPU_USED;  /*  FPU使用フラグをクリアする  */
	}
	x86_64_enable_fpu_task_switch(); /* 次回FPU命令使用時に例外を発生  */
}
