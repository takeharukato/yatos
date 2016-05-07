/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Architecture dependent processor relevant routines                */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/thread.h>
#include <kern/proc.h>
#include <kern/cpu.h>
#include <kern/page.h>

#include <proc/proc-internal.h>

extern void x86_64_prepare(uint64_t _magic, uint64_t _mbaddr);
extern void x86_64_fxsave(void *_m);
extern void x86_64_fxrestore(void *_m);

static proc kproc;
static x86_64_cpu acpus[NR_CPUS] = {__X86_64_CPU_INITIALIZER,};
static idt_descriptor *idtp;

/** CPU固有なGDT/TSSを設定する
 */
static void
setup_current_gdt_tss(void) {
	x86_64_cpu     *ac;
	void  *gdtp = NULL;
	tss64 *tssp = NULL;
	
	kassert( current_cpu() < NR_CPUS );
	ac = &acpus[current_cpu()];

	get_free_page(&gdtp);
	kassert(gdtp != NULL);

	init_segments(gdtp, (tss64 **)&tssp);

	ac->gdtp = gdtp;
	ac->tssp = tssp;
}

/** カーネルプロセス空間情報を初期化する
    @param[in] kpgtbl カーネルのページテーブル
 */
static void
x86_64_init_kernel_proc(void *kpgtbl) {
	proc *p;

	p = &kproc;
	memset(p, 0, sizeof(proc));

	_proc_common_init(p);

	p->vm.pgtbl = kpgtbl;
	p->pid = THR_IDLE_TID;

	p->entry = (int (*)(void *)) x86_64_prepare;

	spinlock_init(&p->lock);
	queue_init(&p->threads);
}

/** FPUコンテキストをCPUから取得する
    @param[in] dest FPUコンテキスト出力先アドレス
    @note fxsave/fxrstor命令は16バイト境界の領域にしか書き込めないので
    16バイト境界の一時バッファに保存してからコピーしている
 */
void
x86_64_fpuctx_save(void *dest) {
	x86_64_cpu   *ac;

	kassert( current_cpu() < NR_CPUS );

	ac = &acpus[current_cpu()];
	x86_64_fxsave( &ac->fpuctxbuf );
	memcpy(dest, &ac->fpuctxbuf, sizeof(fpu_context));
}
/** FPUコンテキストを復元する
    @param[in] dest FPUコンテキスト入力元アドレス
    @note fxsave/fxrstor命令は16バイト境界の領域にしか書き込めないので
    16バイト境界の一時バッファに保存してからコピーしている
 */
void
x86_64_fpuctx_restore(void *src) {
	x86_64_cpu   *ac;

	kassert( current_cpu() < NR_CPUS );
	kassert( src != NULL );

	ac = &acpus[current_cpu()];

	memcpy(&ac->fpuctxbuf, src, sizeof(fpu_context));
	x86_64_fxrestore( &ac->fpuctxbuf );
}

/** udelayの設定値を格納する
    @param[in] tsc 設定するTSC値(1マイクロ秒当たりのtsc値)
 */
void
_x86_64_set_tsc_per_us(uint64_t tsc) {
	x86_64_cpu   *ac;

	kassert( current_cpu() < NR_CPUS );

	ac = &acpus[current_cpu()];
	ac->tsc_per_us = tsc;
}

/** udelayの設定値を獲得する
 */
uint64_t 
_x86_64_get_tsc_per_us(void) {
	x86_64_cpu   *ac;

	kassert( current_cpu() < NR_CPUS );

	ac = &acpus[current_cpu()];

	return ac->tsc_per_us;
}

/** 例外スタックを設定する
    @param[in] ksp 設定するカーネルスタックのアドレス
 */
void
hal_set_exception_stack(void *ksp){
	x86_64_cpu   *ac;
	tss64      *tssp;

	kassert( current_cpu() < NR_CPUS );

	ac = &acpus[current_cpu()];
	tssp = ac->tssp;
	kassert(ac->tssp != NULL);

	tssp->rsp0 = (uint64_t)ksp;
}

/** アドレス空間を切り替える
    @param[in] prev 切り替え前のプロセス
    @param[in] next 切り替え先のプロセス
 */
void
hal_switch_address_space(struct _proc __attribute__ ((unused)) *prev, 
    struct _proc *next) {
	vm *next_as;

	next_as = &next->vm;
	invalidate_tlb();
	load_pgtbl(KERN_STRAIGHT_TO_PHY(next_as->pgtbl));
}

/** カーネル空間のプロセス情報を得る
 */
struct _proc *
hal_refer_kernel_proc(void) {

	return (proc *)(&kproc);
}

/** CPUテーブルの初期化
    @param[in] kpgtbl カーネルのページテーブル
    @note HAL内部から呼ばれる
 */
void
x86_64_init_cpus(void *kpgtbl){
	int          i;
	x86_64_cpu *ac;

	kassert( kpgtbl != NULL);

	for( i = 0; i < NR_CPUS; ++i) {

		ac = &acpus[i];
		memset(ac, 0, sizeof(x86_64_cpu));
	}

	x86_64_init_kernel_proc(kpgtbl);

	setup_current_gdt_tss();
	init_idt((idt_descriptor **)&idtp);
}
