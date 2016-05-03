/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  architechture dependent cpu definitions                           */
/*                                                                    */
/**********************************************************************/
#if !defined(__HAL_ARCH_CPU_H)
#define __HAL_ARCH_CPU_H

#define X86_64_RFLAGS_CF   (0) /*< キャリーフラグ */
#define X86_64_RFLAGS_PF   (2) /*< パリティフラグ (演算結果の下位8ビット) */
#define X86_64_RFLAGS_AF   (4) /*< 補助キャリーフラグ(BCD演算用) */
#define X86_64_RFLAGS_ZF   (6) /*< ゼロフラグ */
#define X86_64_RFLAGS_SF   (7) /*< サインフラグ */
#define X86_64_RFLAGS_TF   (8) /*< トラップフラグ、トレースフラグ */
#define X86_64_RFLAGS_IF   (9) /*< 割り込みフラグ */
#define X86_64_RFLAGS_DF  (10) /*< ディレクションフラグ */
#define X86_64_RFLAGS_OF  (11) /*< オーバーフローフラグ */
#define X86_64_RFLAGS_IO1 (12) /*< IOPL1 */
#define X86_64_RFLAGS_IO2 (13) /*< IOPL2 */
#define X86_64_RFLAGS_NT  (14) /*< ネストタスクフラグ */
#define X86_64_RFLAGS_RF  (16) /*< デバッグレジスターの命令ブレイクポイントを(1回のみ)無効にする */
#define X86_64_RFLAGS_VM  (17) /*< 仮想86モード */
#define X86_64_RFLAGS_AC  (18) /*< 変更可能であれば、i486、Pentium以降のCPUである */
#define X86_64_RFLAGS_VIF (19) /*< 仮想割り込みフラグ (Pentium以降) */
#define X86_64_RFLAGS_VIP (20) /*< 仮想割り込みペンディングフラグ (Pentium以降) */
#define X86_64_RFLAGS_ID  (21) /*< 変更可能であれば、CPUID命令に対応している */
#define X86_64_RFLAGS_NR  (22) /*< 最終位置  */
#define X86_64_RFLAGS_RESV ( ( 1 << 1) | ( 1 << 3) | ( 1 << 5 ) )

#define MISC_ENABLE             (0x000001A0)
#define EFER                    (0xC0000080)

#define CR0_PAGING              (1 << 31)
#define CR0_CACHE_DISABLE       (1 << 30)
#define CR0_NOT_WRITE_THROUGH   (1 << 29)
#define CR0_ALIGNMENT_MASK      (1 << 18)
#define CR0_WRITE_PROTECT       (1 << 16)
#define CR0_NUMERIC_ERROR       (1 << 5)
#define CR0_EXTENSION_TYPE      (1 << 4)
#define CR0_TASK_SWITCHED       (1 << 3)
#define CR0_FPU_EMULATION       (1 << 2)
#define CR0_MONITOR_FPU         (1 << 1)
#define CR0_PROTECTION          (1 << 0)

#define CR4_VME                 (1 << 0)
#define CR4_PMODE_VIRTUAL_INT   (1 << 1)
#define CR4_TIMESTAMP_RESTRICT  (1 << 2)
#define CR4_DEBUGGING_EXT       (1 << 3)
#define CR4_PSE                 (1 << 4)
#define CR4_PAE                 (1 << 5)
#define CR4_MCE                 (1 << 6)
#define CR4_GLOBAL_PAGES        (1 << 7)
#define CR4_PERF_COUNTER        (1 << 8)
#define CR4_OS_FXSR             (1 << 9)
#define CR4_OS_XMMEXCEPT        (1 << 10)
#define CR4_VMX_ENABLE          (1 << 13)
#define CR4_SMX_ENABLE          (1 << 14)
#define CR4_PCID_ENABLE         (1 << 17)
#define CR4_OS_XSAVE            (1 << 18)

#if !defined(ASM_FILE)
#include <stdint.h>
#include <kern/config.h>
#include <kern/thread-info.h>
#include <hal/segment.h>

#define TI_X86_64_FPU_USED (0x1)  /*  FPU使用済みフラグ  */

typedef struct _fpu_context{
	uint16_t            fcw;
	uint16_t            fsw;
	uint8_t             ftw;
	uint8_t            pad1;
	uint16_t            fop;
	uint32_t         fpu_ip;
	uint16_t             cp;
	uint16_t          resv1;
	uint32_t         fpu_dp;
	uint16_t             ds;
	uint16_t          resv2;
	uint32_t          mxcsr;
	uint32_t     mxcsr_mask;
	uint64_t        mm0_low;
	uint64_t       mm0_high;
	uint64_t        mm1_low;
	uint64_t       mm1_high;
	uint64_t        mm2_low;
	uint64_t       mm2_high;
	uint64_t        mm3_low;
	uint64_t       mm3_high;
	uint64_t        mm4_low;
	uint64_t       mm4_high;
	uint64_t        mm5_low;
	uint64_t       mm5_high;
	uint64_t        mm6_low;
	uint64_t       mm6_high;
	uint64_t        mm7_low;
	uint64_t       mm7_high;
	uint64_t       mmx0_low;
	uint64_t      mmx0_high;
	uint64_t       mmx1_low;
	uint64_t      mmx1_high;
	uint64_t       mmx2_low;
	uint64_t      mmx2_high;
	uint64_t       mmx3_low;
	uint64_t      mmx3_high;
	uint64_t       mmx4_low;
	uint64_t      mmx4_high;
	uint64_t       mmx5_low;
	uint64_t      mmx5_high;
	uint64_t       mmx6_low;
	uint64_t      mmx6_high;
	uint64_t       mmx7_low;
	uint64_t      mmx7_high;
	uint64_t  mmx_resv1_low;
	uint64_t mmx_resv1_high;
	uint64_t  mmx_resv2_low;
	uint64_t mmx_resv2_high;
	uint64_t  mmx_resv3_low;
	uint64_t mmx_resv3_high;
	uint64_t  mmx_resv4_low;
	uint64_t mmx_resv4_high;
	uint64_t  mmx_resv5_low;
	uint64_t mmx_resv5_high;
	uint64_t  mmx_resv6_low;
	uint64_t mmx_resv6_high;
	uint64_t  mmx_resv7_low;
	uint64_t mmx_resv7_high;
	uint64_t  mmx_resv8_low;
	uint64_t mmx_resv8_high;
	uint64_t  mmx_resv9_low;
	uint64_t mmx_resv9_high;
	uint64_t  mmx_resv10_low;
	uint64_t mmx_resv10_high;
	uint64_t  mmx_resv11_low;
	uint64_t mmx_resv11_high;
	uint64_t  mmx_resv12_low;
	uint64_t mmx_resv12_high;
	uint64_t  mmx_resv13_low;
	uint64_t mmx_resv13_high;
} __attribute__((packed))  __attribute__((aligned(16))) fpu_context;


typedef struct _x86_64_cpu{
	void              *gdtp;
	void              *tssp;
	uint64_t     tsc_per_us;
}x86_64_cpu;

#define __X86_64_CPU_INITIALIZER    \
	{			    \
	.gdtp = NULL,		    \
	.tssp = NULL,	            \
	.tsc_per_us = 0,            \
	}

static inline void
wrmsr(uint32_t msr_id, uint64_t msr_value){

	asm volatile ( "wrmsr" : : "c" (msr_id), "A" (msr_value) );
}

static inline uint64_t 
rdmsr(uint32_t msr_id) {
	uint64_t msr_value;

	asm volatile ( "rdmsr" : "=A" (msr_value) : "c" (msr_id) );

	return msr_value;
}

struct _cpu;
void arch_setup_cpuinfo(int _cpuid, struct _cpu *_c);
void x86_64_fxsave(void *_m);
void x86_64_fxrestore(void *_m);
void x86_64_enable_fpu_task_switch(void);
void x86_64_disable_fpu_task_switch(void);
#endif  /*  !ASM_FILE  */

#endif  /*  __HAL_ARCH_CPU_H  */
