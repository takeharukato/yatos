/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  trap handler routines                                             */
/*                                                                    */
/**********************************************************************/
#include <stddef.h>

#include <kern/config.h>
#include <kern/assert.h>
#include <kern/string.h>
#include <kern/kprintf.h>
#include <kern/spinlock.h>
#include <kern/errno.h>
#include <kern/proc.h>
#include <kern/thread.h>
#include <kern/vm.h>
#include <kern/irq.h>

#include <hal/segment.h>
#include <hal/arch-cpu.h>

//#define DEBUG_TRAP_WITH_INT3

/** トラップ名
 */
static const char *trapnames[] = {
	"Divide-by-zero Error",
	"Debug",
	"Non-maskable Interrupt",
	"Breakpoint",
	"Overflow",
	"Bound Range Exceeded",
	"Invalid Opcode",
	"Device Not Available",
	"Double Fault",
	"Coprocessor Segment Overrun",
	"Invalid TSS",
	"Segment Not Present",
	"Stack-Segment Fault",
	"General Protection Fault",
	"Page Fault",
	"Reserved",
	"x87 Floating-Point Exception",
	"Alignment Check",
	"Machine Check",
	"SIMD Floating-Point Exception",
	"Virtualization Exception",
	"Reserved",
	"Security Exception",
	"Reserved",
};

/** RFLAGSの意味
 */
static const char *rfbit2str[] = {
	"CF",
	"RSV",
	"PF",
	"RSV",
	"AF",
	"RSV",
	"ZF",
	"SF",
	"TF",
	"IF",
	"DF",
	"OF",
	"IOPL1",
	"IOPL2",
	"NT",
	"RSV",
	"RF",
	"VM",
	"AC",
	"VIF",
	"VIP",
	"ID",
};

/** トラップ時のコンテキスト情報を表示する
    @param[in] ctx トラップ時のコンテキスト
 */
static void 
show_trap_context(trap_context *ctx) {
	int       i;
	uint64_t rf;

	kassert(ctx != NULL);
	if ( ctx->trapno < X86_NR_EXCEPTIONS )
		kprintf(KERN_INF, "%s:\n", trapnames[ctx->trapno]);
	else
		kprintf(KERN_INF, "UnknownTrap:\n", trapnames[ctx->trapno]);

	kprintf(KERN_INF, "Trap: 0x%x(%d) ErrorCode: 0x%016lx\n", 
	    ctx->trapno, ctx->trapno, ctx->errno);
	kprintf(KERN_INF, "PageTable: %016lx\n", read_cr3());
	if ( ctx->trapno == X86_PAGE_FAULT ) {

		kprintf(KERN_INF, "Reason: [%c%c%c%c] FaultAddress: %016lx\n", 
		    ( ctx->errno & PGFLT_ECODE_INSTPREF ) ? ('I') : (' '), 
		    ( ctx->errno & PGFLT_ECODE_USER  ) ? ('U') : (' '),
		    ( ctx->errno & PGFLT_ECODE_WRITE ) ? ('W') : (' '),
		    ( ctx->errno & PGFLT_ECODE_PROT  ) ? ('P') : (' '), 
		    read_cr2());
	}

	if ( hal_is_intr_from_user(ctx) )
		kprintf(KERN_INF, "RIP: 0x%02x:0x%016lx RSP: 0x%02x:0x%016lx\n",
		    (uint16_t)ctx->cs, ctx->rip, (uint16_t)ctx->ss, ctx->rsp);
	else
		kprintf(KERN_INF, "RIP: 0x%02x:0x%016lx RSP: 0x%016lx\n",
		    (uint16_t)ctx->cs, ctx->rip,
		    ( ( (uintptr_t)ctx->rsp) - sizeof(trap_context) ) );

	kprintf(KERN_INF, "RFLAGS: 0x%016lx IOPL[%d] ", ctx->rflags, 
	    ( ctx->rflags >> X86_64_RFLAGS_IO1 ) & 3 );
	rf = ctx->rflags & ~(uint64_t)X86_64_RFLAGS_RESV;
	for(i = X86_64_RFLAGS_NR - 1; i > 0; --i) {

		if ( ( i == X86_64_RFLAGS_IO1 ) || ( i == X86_64_RFLAGS_IO2 ) )
			continue;
		else if ( rf & ( 1 << i ) )
			kprintf(KERN_INF, "%s ", rfbit2str[i]);
	}
	kprintf(KERN_INF, "\n");

	kprintf(KERN_INF, "RAX: 0x%016lx RBX: 0x%016lx RCX: 0x%016lx RDX: 0x%016lx\n",
	    ctx->rax, ctx->rbx, ctx->rcx, ctx->rdx);
	kprintf(KERN_INF, "RBP: 0x%016lx RSI: 0x%016lx RDI: 0x%016lx\n",
	    ctx->rbp, ctx->rsi, ctx->rdi);
	kprintf(KERN_INF, "R8 : 0x%016lx R9 : 0x%016lx R10: 0x%016lx R11: 0x%016lx\n",
	    ctx->r8, ctx->r9, ctx->r10, ctx->r11);
	kprintf(KERN_INF, "R12: 0x%016lx R13: 0x%016lx R14: 0x%016lx R15: 0x%016lx\n",
	    ctx->r12, ctx->r13, ctx->r14, ctx->r15);

	kprintf(KERN_INF, "[Trace]\n");
	if ( hal_is_intr_from_user(ctx) )
		print_back_trace((void *)NULL);
	else
		print_back_trace((void *)ctx->rbp);
}

/**  不正処理/不正トラップによる自スレッド終了
     @param[in] ctx トラップ時のコンテキスト
 */
static void
x86_64_trap_exit(trap_context *ctx) {

	kassert( ctx != NULL );

	kprintf(KERN_INF, "current=%p tid=%d:\n", current, current->tid);
	show_trap_context(ctx);	
	kprintf(KERN_INF, "KILLED current=%p tid=%d:\n", 
	    current, current->tid);
	thr_exit(ctx->trapno);   /*  自スレッド終了  */
}

/**  エラー情報表示とシステムの停止
     @param[in] ctx トラップ時のコンテキスト
 */
static void
handle_error_exception(trap_context *ctx) {

	show_trap_context(ctx);	
	while(1);
}

static void
divide_error(trap_context *ctx) {
	int           rc;
	event_node *node;

	if ( hal_is_intr_from_user(ctx) ) {

		kassert(current->type == THR_TYPE_USER);

		rc = ev_alloc_node(EV_SIG_FPE, &node);
		if ( rc != 0 ) 
			goto exit_out;

		node->info.code = EV_CODE_FPE_INTDIV;
		node->info.trap = ctx->trapno;
		node->info.err = ctx->errno;

		rc = ev_send(current->tid, node);
		if ( rc != 0 )
			goto exit_out;
	} else {

		handle_error_exception(ctx);
	}

	return;

exit_out:
	x86_64_trap_exit(ctx);
}

static void
debug(trap_context __attribute__ ((unused)) *ctx){

}

static void
nmi(trap_context *ctx){

	kprintf(KERN_INF, "NMI:\n");
	handle_error_exception(ctx);
}


static void
int3(trap_context __attribute__ ((unused)) *ctx) {

#if defined(DEBUG_TRAP_WITH_INT3)
	kprintf(KERN_INF, "INT3:\n");
	show_trap_context(ctx);
#endif  /*  DEBUG_TRAP_WITH_INT3  */
}

static void
overflow(trap_context *ctx) {
	int           rc;
	event_node *node;

	if ( hal_is_intr_from_user(ctx) ) {

		kassert(current->type == THR_TYPE_USER);

		rc = ev_alloc_node(EV_SIG_FPE, &node);
		if ( rc != 0 ) 
			goto exit_out;

		node->info.code = EV_CODE_FPE_INTOVF;
		node->info.trap = ctx->trapno;
		node->info.err = ctx->errno;

		rc = ev_send(current->tid, node);
		if ( rc != 0 )
			goto exit_out;
	} else {

		handle_error_exception(ctx);
	}

	return;

exit_out:
	x86_64_trap_exit(ctx);
}

static void
bounds(trap_context *ctx) {
	int           rc;
	event_node *node;

	if ( hal_is_intr_from_user(ctx) ) {

		kassert(current->type == THR_TYPE_USER);

		rc = ev_alloc_node(EV_SIG_SEGV, &node);
		if ( rc != 0 ) 
			goto exit_out;

		node->info.code = EV_CODE_SEGV_MAPERR;
		node->info.trap = ctx->trapno;
		node->info.err = ctx->errno;

		rc = ev_send(current->tid, node);
		if ( rc != 0 )
			goto exit_out;
	} else {

		handle_error_exception(ctx);
	}

	return;

exit_out:
	x86_64_trap_exit(ctx);
}

static void
invalid_op(trap_context *ctx) {
	int           rc;
	event_node *node;

	if ( hal_is_intr_from_user(ctx) ) {

		kassert(current->type == THR_TYPE_USER);

		rc = ev_alloc_node(EV_SIG_ILL, &node);
		if ( rc != 0 ) 
			goto exit_out;

		node->info.code = EV_CODE_ILL_ILLOPN;
		node->info.trap = ctx->trapno;
		node->info.err = ctx->errno;

		rc = ev_send(current->tid, node);
		if ( rc != 0 )
			goto exit_out;
	} else {

		handle_error_exception(ctx);
	}

	return;

exit_out:
	x86_64_trap_exit(ctx);
}

/** デバイス不在例外ハンドラ
    @param[in] ctx 例外コンテキスト
    @note コンテキストスイッチ後初めて, x87 FPU命令を使用した時点で
    浮動小数点レジスタの復元を行う。
 */
static void
device_not_available(trap_context __attribute__ ((unused)) *ctx) {
	thread_info *ti;

	ti = ti_get_current_tinfo();
	x86_64_disable_fpu_task_switch(); /*  スレッド切り替えまで例外抑止  */
	ti->arch_flags |= TI_X86_64_FPU_USED;
	x86_64_fpuctx_restore( &current->fpctx ); /*  FPUコンテキストを復元する  */
}

static void
double_fault(trap_context *ctx) {

	handle_error_exception(ctx);
}

static void
coprocessor_segment_overrun(trap_context *ctx) {

	handle_error_exception(ctx);
}

static void
invalid_TSS(trap_context *ctx) {

	handle_error_exception(ctx);
}

static void
segment_not_present(trap_context *ctx) {

	handle_error_exception(ctx);
}

static void
stack_segment(trap_context *ctx) {
	int           rc;
	event_node *node;

	if ( hal_is_intr_from_user(ctx) ) {

		kassert(current->type == THR_TYPE_USER);

		rc = ev_alloc_node(EV_SIG_SEGV, &node);
		if ( rc != 0 ) 
			goto exit_out;

		node->info.code = EV_CODE_SEGV_ACCERR;
		node->info.trap = ctx->trapno;
		node->info.err = ctx->errno;

		rc = ev_send(current->tid, node);
		if ( rc != 0 )
			goto exit_out;
	} else {

		handle_error_exception(ctx);
	}

	return;

exit_out:
	x86_64_trap_exit(ctx);
}

static void
general_protection(trap_context *ctx) {
	int           rc;
	event_node *node;

	if ( hal_is_intr_from_user(ctx) ) {

		kassert(current->type == THR_TYPE_USER);

		rc = ev_alloc_node(EV_SIG_SEGV, &node);
		if ( rc != 0 ) 
			goto exit_out;

		node->info.code = EV_CODE_SEGV_ACCERR;
		node->info.trap = ctx->trapno;
		node->info.err = ctx->errno;

		rc = ev_send(current->tid, node);
		if ( rc != 0 )
			goto exit_out;
	} else {

		handle_error_exception(ctx);
	}

	return;

exit_out:
	x86_64_trap_exit(ctx);
}

static void
page_fault_exception(trap_context *ctx) {
	int           rc;
	vma        *vmap;
	void *fault_addr;

	if ( hal_is_intr_from_user(ctx) ) {
		
		kassert(current->type == THR_TYPE_USER);

		fault_addr = (void *)read_cr2();

		rc = vm_find_vma( &current->p->vm, fault_addr, &vmap);
		if ( rc == 0 ) {

			rc = vm_map_newpage(&current->p->vm, fault_addr, vmap->prot);
			if ( rc != 0 )
				goto exit_out;
		}

		/*
		 * Stack fault
		 */
		if ( ( current->p->heap->end < fault_addr ) &&
		    ( fault_addr < current->p->stack->start ) ) {
			
			rc = proc_expand_stack(current->p, fault_addr);
			if ( rc != 0 )
				goto exit_out;
		}
	} else {
		
		handle_error_exception(ctx);
	}

	return;

exit_out:
	x86_64_trap_exit(ctx);
}

static void
coprocessor_error(trap_context *ctx) {

	handle_error_exception(ctx);
}

static void
alignment_check_error(trap_context *ctx) {
	int           rc;
	event_node *node;

	if ( hal_is_intr_from_user(ctx) ) {

		kassert(current->type == THR_TYPE_USER);

		rc = ev_alloc_node(EV_SIG_BUS, &node);
		if ( rc != 0 ) 
			goto exit_out;

		node->info.code = EV_CODE_BUS_ADRALN;
		node->info.trap = ctx->trapno;
		node->info.err = ctx->errno;

		rc = ev_send(current->tid, node);
		if ( rc != 0 )
			goto exit_out;
	} else {

		handle_error_exception(ctx);
	}

	return;

exit_out:
	x86_64_trap_exit(ctx);
}

static void
machine_check_error(trap_context *ctx) {

	handle_error_exception(ctx);
}

static void
simd_coprocessor_error(trap_context *ctx) {

	handle_error_exception(ctx);
}

static void
cpu_exception(trap_context *ctx) {
	int           rc;
	event_node *node;

	kassert(ctx != NULL);
	
	switch(ctx->trapno) {

	case X86_DIV_ERR:
		
		divide_error(ctx);
		break;

	case X86_DEBUG_EX:

		debug(ctx);
		break;

	case X86_NMI:

		nmi(ctx);
		break;

	case X86_BREAKPOINT:

		int3(ctx);
		break;

	case X86_OVERFLOW:

		overflow(ctx);
		break;

	case X86_BOUND_RANGE:

		bounds(ctx);
		break;

	case X86_INVALID_OP:

		invalid_op(ctx);
		break;

	case X86_DEVICE_NA:

		device_not_available(ctx);
		break;

	case X86_DFAULT:

		double_fault(ctx);
		break;

	case X86_CO_SEG_OF:

		coprocessor_segment_overrun(ctx);
		break;

	case X86_INVALID_TSS:

		invalid_TSS(ctx);
		break;
	case X86_SEG_NP:

		segment_not_present(ctx);
		break;

	case X86_STACK_FAULT:

		stack_segment(ctx);
		break;

	case X86_GPF:

		general_protection(ctx);
		break;

	case X86_PAGE_FAULT:

		page_fault_exception(ctx);
		break;

	case X86_FPE:

		coprocessor_error(ctx);
		break;

	case X86_ALIGN_CHECK:

		alignment_check_error(ctx);
		break;

	case X86_MCE:

		machine_check_error(ctx);
		break;

	case X86_SIMD_FPE:

		simd_coprocessor_error(ctx);
		break;
	default:

		/*
		 * 不正トラップ
		 */
		if ( hal_is_intr_from_user(ctx) ) {

			show_trap_context(ctx);	
			kassert(current->type == THR_TYPE_USER);

			rc = ev_alloc_node(EV_SIG_ILL, &node);
			if ( rc != 0 ) 
				goto exit_out;

			node->info.code = EV_CODE_ILL_ILLOPN;
			node->info.trap = ctx->trapno;
			node->info.err = ctx->errno;

			rc = ev_send(current->tid, node);
			if ( rc != 0 )
				goto exit_out;
		} else {
			
			handle_error_exception(ctx);
		}
		break;
	}

	return;

exit_out:
	x86_64_trap_exit(ctx);
}

static void
handle_syscall_trap(trap_context *ctx){

	kassert(ctx != NULL);	

	x86_64_dispatch_syscall(ctx);
}

static void
handle_interrupt(trap_context *ctx){

	kassert(ctx != NULL);	

	kassert( I8259_PIC1_VBASE_ADDR <= ctx->trapno );
	
	kcom_handle_irqs(ctx->trapno - I8259_PIC1_VBASE_ADDR, ctx);
}

void
trap_common(trap_context *ctx){

	kassert(ctx != NULL);	
	
	if (ctx->trapno == TRAP_SYSCALL)
		handle_syscall_trap(ctx);
	else if ( ctx->trapno <= X86_SIMD_FPE )
		cpu_exception(ctx);
	else 
		handle_interrupt(ctx);
}
