/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  System call dispatcher routines                                   */
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
#include <kern/async-event.h>
#include <kern/svc.h>

#include <hal/traps.h>

extern void _x86_64_return_from_event_handler(event_frame *_user_ef, trap_context *_ctx);

/** システムコールをディスパッチする
    @param[in] ctx コンテキスト情報
 */
void
x86_64_dispatch_syscall(trap_context *ctx) {
	int        rc;
	uint64_t   no;

	/* システムコールのレジスタコンベンション
	 * システムコール番号      rax
	 * 第1引数                 rdi
	 * 第2引数                 rsi
	 * 第3引数                 rdx
	 * 第4引数                 r10
	 * 第5引数(未使用)          r8
	*/

	no = ctx->rax;
	switch( no ) {

	case SYS_YATOS_EV_REG_EVHANDLER:

		rc = svc_register_common_event_handler((void *)ctx->rdi);
		ctx->rax = (uint64_t)rc;
		break;

	case SYS_YATOS_EV_RETURN:
		_x86_64_return_from_event_handler((event_frame *)ctx->rdi, ctx);
		break;

	case SYS_YATOS_THR_YIELD:
		rc = svc_thr_yield();
		ctx->rax = (uint64_t)rc;
		break;
	case SYS_YATOS_THR_EXIT:
		svc_thr_exit(ctx->rdi);
		ctx->rax = 0;
		break;
	case SYS_YATOS_THR_GETID:
		ctx->rax = svc_thr_getid();
		break;
	case SYS_YATOS_THR_WAIT:
		ctx->rax = svc_thr_wait((tid)ctx->rdi, (thread_wait_flags)ctx->rsi, 
		    (tid *)ctx->rdx, (exit_code *)ctx->r10);
		break;
	case SYS_YATOS_LPC_SEND:
		rc = svc_lpc_send(ctx->rdi, ctx->rsi, (void *)ctx->rdx);
		ctx->rax = (uint64_t)rc;
		break;

	case SYS_YATOS_LPC_RECV:
		rc = svc_lpc_recv(ctx->rdi, ctx->rsi, (void *)ctx->rdx, (void *)ctx->r10);
		ctx->rax = (uint64_t)rc;
		break;
	case SYS_YATOS_LPC_SEND_AND_REPLY:
		rc = svc_lpc_send_and_reply(ctx->rdi, (void *)ctx->rsi);
		ctx->rax = (uint64_t)rc;
		break;
	default:
		ctx->rax = (uint64_t)-ENOSYS;
		break;
	}
}
