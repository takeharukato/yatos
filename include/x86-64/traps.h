/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  trapframe definitions                                             */
/*                                                                    */
/**********************************************************************/
#if !defined(__HAL_TRAPS_H)
#define __HAL_TRAPS_H
#include <stdint.h>

#define X86_DIV_ERR       (0)
#define X86_DEBUG_EX      (1)
#define X86_NMI           (2)
#define X86_BREAKPOINT    (3)
#define X86_OVERFLOW      (4)
#define X86_BOUND_RANGE   (5)
#define X86_INVALID_OP    (6)
#define X86_DEVICE_NA     (7)
#define X86_DFAULT        (8)
#define X86_CO_SEG_OF     (9)
#define X86_INVALID_TSS   (10)
#define X86_SEG_NP        (11)
#define X86_STACK_FAULT   (12)
#define X86_GPF           (13)
#define X86_PAGE_FAULT    (14)
#define X86_FPE           (16)
#define X86_ALIGN_CHECK   (17)
#define X86_MCE           (18)
#define X86_SIMD_FPE      (19)

#define X86_NR_EXCEPTIONS (20)

#define TRAP_SYSCALL  (0x90)
#define NR_TRAPS      (256)

/** ページフォルトエラーコード
 */
#define PGFLT_ECODE_PROT     (1)  /*<  プロテクションフォルト  */
#define PGFLT_ECODE_WRITE    (2)  /*<  書込みフォルト          */
#define PGFLT_ECODE_USER     (4)  /*<  ユーザフォルト          */
#define PGFLT_ECODE_RSV      (8)  /*<  予約                    */
#define PGFLT_ECODE_INSTPREF (16) /*<  命令プリフェッチ        */

typedef struct _trap_context{
	uint64_t rax;
	uint64_t rbx;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rbp;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t  r8;
	uint64_t  r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t trapno;
	uint64_t errno;
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
}trap_context;

void x86_64_dispatch_syscall(trap_context *_ctx);

#endif  /*  __HAL_TRAPS_H  */
