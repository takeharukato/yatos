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

//#define DEBUG_BOOT_TIME_PAGE_POOL

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

/** 高位メモリページ(3GiBより大きなアドレス)を登録する
    @param[in] info ブート時情報
 */
static void
x86_64_add_high_pages(karch_info  *info) {
	int            i;
	memory_area  *ma;

	kassert( info != NULL );

	for( i = 0; info->nr_ram > i; ++i ) {
		
		ma =  &info->ram_area[i];
		if ( info->mem_upper_kb * 1024UL < ma->start ) {
			
			x86_64_alloc_page_info(ma->start,
			    ( ma->end > KERN_PHY_MAX ) ?
			    ( KERN_PHY_MAX ) : ma->end );
		}
	}
}

/** メモリページ/ストレートマップ領域の初期化
    @param[in] info ブート時情報
 */
static void 
page_init(karch_info  *info) {
	obj_cnt_type nr_free_pages;
	obj_cnt_type      nr_pages;

	kassert( info != NULL );

	/* 低位メモリ(3GiB以下, Grubのmem_upperで取得される範囲をストレートマップ 
	   @note 0-640KiBまでの物理メモリを利用してストレートマップ用のページテーブル
	   を作成する。
	   カーネルのメモリページは2MiB単位でメモリをマップされ, カーネルプログラム
	   カバーする範囲はページプールの予約として扱われる。
	   予約処理中に含まれるRAMである640KiB未満の物理メモリを使用して、3GiBまでの
	   カーネルストレートマップを作成する。
	 */
	x86_64_boot_map_kernel( info ); 

	/* 低位メモリをページプールに登録 */
	x86_64_alloc_page_info(info->phy_mem_min, info->mem_upper_kb * 1024UL);

	/* これ以降3GiBまでの範囲でページプールからのメモリ獲得が行える  */

	/* ページプール中のメモリを使用して, 搭載メモリの開始アドレスから
	   最終アドレス(5GiB以上2TiB)までをカーネル空間に再マップする */
	x86_64_remap_kernel(info);

	/* 高位メモリページをページプールに登録する  */
	x86_64_add_high_pages(info);

	kcom_refer_free_pages(&nr_pages, &nr_free_pages);

#if defined(DEBUG_BOOT_TIME_PAGE_POOL)
	kprintf(KERN_INF, "page-pool: %d/%d free pages(%d MiB free)\n", 
	    nr_free_pages, 
	    nr_pages,
	    (nr_free_pages << PAGE_SHIFT)  / 1024 / 1024);
#endif  /*  DEBUG_BOOT_TIME_PAGE_POOL */
}

/** Cエントリルーチン
    アーキ固有の初期化を実施.
 */
void
x86_64_prepare(uint64_t __attribute__ ((unused)) magic, 
    uint64_t __attribute__ ((unused)) mbaddr) {
	karch_info  *info = &boot_info;

	bss_init();
	init_kconsole();
	memset(info, 0, sizeof(karch_info));

	kprintf(KERN_INF, "Initialize...\n");

	x86_64_parse_multiboot2_info(magic, mbaddr, info);

	page_init(info);

	x86_64_boot_acpiinit(info);

	x86_64_init_cpus(info->kpgtbl);

	kcom_start_kernel();

	while(1);
}

/** 起動処理用に予約したリソースを解放する
 */
void
hal_release_boot_time_resources(void) {
	intrflags flags;

	hal_cpu_disable_interrupt( &flags );

	/* ブート時の予約ページを解放  */	
	x86_64_release_boot_reserved_pages(&boot_info);

	hal_cpu_restore_interrupt( &flags );
}
