/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Offset definitions for assembler routines                         */
/*                                                                    */
/**********************************************************************/

#include <kern/asm-offset-helper.h>

#define __IN_ASM_OFFSET 1
#include <kern/kern_types.h>
#include <kern/page.h>
#include <kern/thread.h>
#include <kern/thread-info.h>
#include <hal/traps.h>

int
main(int  __attribute__ ((unused)) argc, char  __attribute__ ((unused)) *argv[]) {

	//DEFINE_SIZE(KERNEL_STACK_INFO_SIZE, sizeof(struct _kernel_stack_info));
	DEFINE_SIZE(THREAD_INFO_SIZE, sizeof(struct _thread_info));

	OFFSET(TI_MAGIC_OFFSET, _thread_info, magic);
	OFFSET(TI_INTRCNT_OFFSET, _thread_info, intrcnt);
	OFFSET(TI_PREEMPT_OFFSET, _thread_info, preempt);
	OFFSET(TI_FLAGS_OFFSET, _thread_info, flags);
	OFFSET(TI_ARCH_FLAGS_OFFSET, _thread_info, arch_flags);
	OFFSET(TI_THREAD_OFFSET, _thread_info, thr);
	OFFSET(TI_CPU_OFFSET, _thread_info, cpu);

	OFFSET(CTX_TRAPNO_OFFSET, _trap_context, trapno);
	OFFSET(CTX_RIP_OFFSET, _trap_context, rip);
	OFFSET(CTX_CS_OFFSET, _trap_context, cs);
	OFFSET(CTX_RFLAGS_OFFSET, _trap_context, rflags);
	OFFSET(CTX_RSP_OFFSET, _trap_context, rsp);
	OFFSET(CTX_SS_OFFSET, _trap_context, ss);

	OFFSET(THR_FPCTX_OFFSET, _thread, fpctx);

	DEFINE_SIZE(PAGE_FRAME_SIZE, sizeof(struct _page_frame));

	DEFINE_SIZE(EV_BITMAP_SIZE, sizeof(events_map));
	return 0;
}
