/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  X86-64 specific context switch codes                              */
/*                                                                    */
/**********************************************************************/

#define ASM_FILE   1
#define THR_FUNCTION_OFFSET  8  /*< スタック内での関数アドレス格納位置  */
#define THR_ARG_OFFSET      16  /*< スタック内での引数格納位置          */

	.section .text
	.globl	x86_64_start_kthread, kcom_launch_new_thread, hal_do_context_switch
/** x86-64用のカーネルスレッドスタート関数呼び出しルーチン
	@note x86-64は, 引数のレジスタ渡しを徹底するため, スタックからデータを取り出し,
	スタック経由で渡された関数ポインタと引数アドレスをレジスタに設定してから
	kcom_launch_new_thread関数を呼び出す.
 */
x86_64_start_kthread:
	movq  THR_FUNCTION_OFFSET(%rsp), %rdi
	movq  THR_ARG_OFFSET(%rsp), %rsi
        movabsq $kcom_launch_new_thread, %r11
        jmp *%r11
	retq  /* ここには来ないはずだが, 来た場合は, thr_exit_thread にジャンプする  */

/** X86-64のスタック切り替え
    @param[in] rdi 第1引数 切り替えられるスレッドのスタックアドレスを格納しているポインタ変数のアドレス
    @param[in] rsi 第2引数 切り替えるスレッドのスタックアドレスを格納しているポインタ変数のアドレス
    @note PSW(Processor Status Word - X86-64の場合, RFLAGSとRIPの組)と
          AMD64 ABI Draft 0.99.5(http://www.x86-64.org/documentation/abi.pdf)で規定された
          callee savedレジスタのうち x87 FPU 制御ワード以外のレジスタを退避/復元する
	  x86-64のcallee savedレジスタ(AMD64 ABI Draft 0.99.5 Figure 3.4: Register Usage参照)
 */
hal_do_context_switch:	
	pushq %rbp
	mov   %rsp, %rbp
	pushfq
	pushq %rbx
	pushq %rbp
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	movabsq $1f, %r11
	pushq %r11
	movq %rsp, (%rdi)
	movq (%rsi), %rsp
	retq
1:
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbp
	popq %rbx
	popfq
	leaveq
	retq

