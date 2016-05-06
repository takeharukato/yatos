/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  page pool routines                                                */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/spinlock.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/errno.h>
#include <kern/page.h>
#include <kern/queue.h>

#include <hal/kernlayout.h>
#include <hal/prepare.h>

extern karch_info  *_refer_boot_info(void);

static int
check_page_reserved(karch_info *info, uintptr_t paddr) {
	int i;

	if ( paddr < 0x1000)
		return 1; /* Zero page  */

	if ( ( 0xa0000 <= paddr ) && ( paddr < 0x100000) ){

		return 1; /* Video memory  */
	}
	

	if ( ( 0xc0000000 <= paddr ) && ( paddr < 0x0000000100000000) ) {

		return 1; /* memory mapped device  */
	}

	if ( ( info->boot_kpgtbl_start_phy <= paddr ) && 
	    ( paddr < ( (uintptr_t)KERN_STRAIGHT_TO_PHY(info->boot_kpgtbl) + PAGE_SIZE ) ) ) {

		return 1; /* kernel page table */
	}

	if ( (  PAGE_START( (uintptr_t)&_kernel_start )  <= paddr ) &&
	    ( paddr < PAGE_END((uintptr_t)&_kernel_end) ) ) {

		return 1; /* kernel page  */
	}

	for(i = 0; i < info->nr_resv; ++i) {
		memory_area *resv = &info->resv_area[i];

		if ( ( resv->start <= paddr ) && ( paddr < resv->end) ) {

			return 1; /* reserved page  */
		}
	}

	for(i = 0; i < info->nr_mod; ++i) {
		grub_mod *mod = &info->modules[i];

		if ( ( mod->start <= paddr ) && ( paddr < mod->end) ) {

			return 1; /* module page  */
		}
	}

	return 0;
}

/** ページフレーム番号からカーネルアドレスを返却する
    @param[in] pfn     ページフレーム番号
    @param[in] kvaddrp カーネルアドレスを返却する領域
    @retval    0      正常に返却した
    @retval   -ENOENT 管理対象外のページフレーム番号を指定した
 */
int
hal_pfn_to_kvaddr(obj_cnt_type pfn, void **kvaddrp) {

	if ( !kcom_is_pfn_valid_nolock(pfn) )
		return -ENOENT;

	*kvaddrp = (void *)PHY_TO_KERN_STRAIGHT( ( ( (uintptr_t)pfn ) << PAGE_SHIFT ) );

	return 0;
}

/** カーネルアドレスからページフレーム番号を返却する
    @param[in] kvaddr   カーネルアドレス
    @param[in] pfnp     ページフレーム番号返却領域
    @retval    0     正常に返却した
    @retval   -ENOENT 管理対象外のアドレスを指定した
 */
int 
hal_kvaddr_to_pfn(void *kvaddr, obj_cnt_type *pfnp) {
	obj_cnt_type       pfn;

	pfn = ( KERN_STRAIGHT_TO_PHY((uintptr_t)kvaddr) ) >> PAGE_SHIFT;

	if ( !kcom_is_pfn_valid(pfn) )
		return -ENOENT;

	*pfnp = pfn;

	return 0;
}

/** ページフレームが予約されていることを確認する
    @param[in] pfn   確認対象のページフレーム番号
    @retval    true  ページが予約されている
    @retval    false ページが予約されていない
 */
bool
hal_is_pfn_reserved(obj_cnt_type pfn) {
	karch_info       *info;
	
	info = _refer_boot_info();

	return check_page_reserved(info, pfn << PAGE_SHIFT);
}

/** ページフレーム情報の配列の領域を確保する
    @param[in] min_paddr 最小物理メモリアドレス
    @param[in] max_paddr 最大物理メモリアドレス
    @note HALの初期化処理から呼ばれる
 */
void
x86_64_alloc_page_info(uintptr_t min_paddr, uintptr_t max_paddr) {
	page_frame       *array;
	page_frame_info    *pfi;
	obj_cnt_type   nr_pages;

	/* 領域内の総ページ数算出(ページフレーム管理情報/
	 * ページフレーム情報の配列を含んだ
	 * ページフレーム配列で管理する対象ページ数) 
	 */
	nr_pages = ( max_paddr - min_paddr ) >> PAGE_SHIFT; 
	
	/*
	 * ページフレーム管理情報/ページプールのエリア/ページフレーム情報の
	 * 配列アドレスの算出
	 */
	array = 
		(page_frame *)( (uintptr_t)PHY_TO_KERN_STRAIGHT(max_paddr) -
		    (uintptr_t)(sizeof(page_frame) * nr_pages) );

	/* ページフレーム管理情報は, ページフレーム情報の配列の直前に配置する  */
	pfi = (void *)array - sizeof(page_frame_info);

	memset(array, 0, 
	    sizeof(page_frame) * nr_pages);  /*  ページフレーム配列のクリア  */

	memset(pfi, 0, 
	    sizeof(page_frame_info));  /*  ページフレーム管理情報のクリア  */

	pfi->array = (void *)array;
	pfi->min_pfn = min_paddr >> PAGE_SHIFT;
	pfi->max_pfn = ( KERN_STRAIGHT_TO_PHY((uintptr_t)pfi) >> PAGE_SHIFT );
	pfi->nr_pages = nr_pages;

	kcom_add_page_info(pfi);  /*  ページプールの登録  */
}

/** ブート時の予約ページを解放する
    @param[in] info  HALのブート情報格納先アドレス
 */
void
x86_64_release_boot_reserved_pages(karch_info *info) {
	int                      i;
	int                     rc;
	uintptr_t            paddr;
	obj_cnt_type nr_free_pages;
	obj_cnt_type      nr_pages;
	grub_mod              *mod;

	kassert( info != NULL );

	/*
	 * 起動時に一時的に使用したカーネル空間用ページテーブルを解放
	 */
	for( paddr = info->boot_kpgtbl_start_phy;
	     paddr < ( (uintptr_t)KERN_STRAIGHT_TO_PHY(info->boot_kpgtbl) + PAGE_SIZE );
	     paddr += PAGE_SIZE) {

		rc = page_release_reservation( paddr >> PAGE_SHIFT );
		kassert( rc == 0 );
	}

	/*
	 * Grubがロードしたシステムプロセスの実行形式を解放
	 */
	for(i = 0; i < info->nr_mod; ++i) {

		mod = &info->modules[i];
		for( paddr = mod->start;
		     paddr < ( PAGE_ALIGNED( mod->end ) 
			 ? ( mod->end ) 
			 : PAGE_NEXT(mod->end) );
		     paddr += PAGE_SIZE) {

			rc = page_release_reservation( paddr >> PAGE_SHIFT );
			kassert( rc == 0 );			
		}
	}

	kcom_refer_free_pages(&nr_pages, &nr_free_pages);
	kprintf(KERN_INF, "page-pool: %d/%d free pages(%d MiB free)\n", 
	    nr_free_pages, 
	    nr_pages,
	    (nr_free_pages << PAGE_SHIFT)  / 1024 / 1024);
}
