/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  X86-64 specific fpu context operation                             */
/*                                                                    */
/**********************************************************************/

#define ASM_FILE   1

#include <hal/arch-cpu.h>

.section .text
	.globl	x86_64_fxsave, x86_64_fxrestore
	.globl  x86_64_enable_fpu_task_switch, x86_64_disable_fpu_task_switch
/** x86-64用のFPU保存処理
 */
x86_64_fxsave:
	pushq %rbp
	mov   %rsp, %rbp
	fxsave (%rdi)
	leaveq
	retq  

/** x86-64用のFPU保存処理
 */
x86_64_fxrestore:
	pushq %rbp
	mov   %rsp, %rbp
	fxrstor (%rdi)
	leaveq
	retq  

/** X86_64のFPU不在例外通知有効化
*/
x86_64_enable_fpu_task_switch:
	pushq %rbp
	mov   %rsp, %rbp
        movq  %cr0, %rax
	orq   $CR0_TASK_SWITCHED, %rax
	movq  %rax, %cr0
	leaveq
	retq  

/** X86_64のFPU不在例外通知無効化
*/
x86_64_disable_fpu_task_switch:
	pushq %rbp
	mov   %rsp, %rbp
	clts
	leaveq
	retq  
