/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  preparation routines                                              */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/page.h>
#include <kern/proc.h>
#include <kern/thread.h>
#include <kern/vm.h>
#include <kern/sched.h>

#include <hal/prepare.h>
#include <hal/kernlayout.h>
#include <hal/kconsole.h>
#include <hal/segment.h>
#include <hal/pgtbl.h>
#include <hal/arch-cpu.h>
#include <hal/boot-acpi.h>

static karch_info boot_info;

void x86_64_init_cpus(void *_kpgtbl);

/** BSSの初期化
 */
static void 
bss_init(void){
	uintptr_t *start = &_bss_start;
	uintptr_t *end =   &_bss_end;

	while(start<end) 
		*start++ = 0x0;
}

/** HAL内部で共通で使用するブート時情報を参照する
 */
karch_info  *
_refer_boot_info(void) {
	
	return &boot_info;
}

/** Cエントリルーチン
    アーキ固有の初期化を実施.
 */
void
x86_64_prepare(uint64_t __attribute__ ((unused)) magic, 
    uint64_t __attribute__ ((unused)) mbaddr) {
	karch_info  *info = (karch_info *)&boot_info;
	uintptr_t  max_mem;
	page_frame_info *pfi;

	bss_init();
	init_kconsole();
	memset(info, 0, sizeof(karch_info));

	kprintf(KERN_INF, "Initialize...\n");

	parse_multiboot2_info(magic, mbaddr, info);

	max_mem = info->mem_upper_kb * 1024UL;

	remap_kernel(info, max_mem - 1);

	alloc_page_info(info, &pfi, 0, max_mem);

	kcom_init_page_info(pfi);

	boot_acpiinit(info);

	x86_64_init_cpus(info->kpgtbl);

	kcom_start_kernel();

	while(1);
}
