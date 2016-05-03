/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Low level trap routines                                           */
/*                                                                    */
/**********************************************************************/

#define ASM_FILE   1
#include <hal/segment.h>
#include <kern/param.h>
#include <kern/thread-info.h>
#include <hal/asm-offset.h>

.code64
.globl build_trap_context
build_trap_context:
	cli
	cld
	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rdi
	pushq %rsi
	pushq %rbp
	pushq %rdx
	pushq %rcx
	pushq %rbx
	pushq %rax

	movq  %rsp, %rdi  
	call trap_common

.globl ret_from_trap, x86_64_retrun_to_user
x86_64_retrun_to_user:
	cli
	/*  多重割込み時は, ret_from_trapにジャンプ  */
	movq	$(KSTACK_SIZE - 1), %r11
	notq	%r11
	andq	%rsp, %r11
	addq	$KSTACK_SIZE, %r11
	subq	$THREAD_INFO_SIZE, %r11
	movq	TI_INTRCNT_OFFSET(%r11), %r9
	cmpq	$0, %r9
	jnz	ret_from_trap
	movq    %rsp, %rdi
	sti
	call    x86_64_handle_post_exception
	cli
	/*  イベントハンドラ処理のため一度例外から復帰する  */
ret_from_trap:
	popq %rax
	popq %rbx
	popq %rcx
	popq %rdx
	popq %rbp
	popq %rsi
	popq %rdi
	popq %r8
	popq %r9
	popq %r10
	popq %r11
	popq %r12
	popq %r13
	popq %r14
	popq %r15
        # remove trap number and errorcode
	add $16, %rsp
	iretq
	
