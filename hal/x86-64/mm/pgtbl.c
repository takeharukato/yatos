/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  page table handling routines                                      */
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
#include <kern/thread.h>
#include <kern/proc.h>
#include <kern/vm.h>

#include <hal/kernlayout.h>
#include <hal/pgtbl.h>

//#define  DEBUG_USER_PGTBL_MAP

/** PTEテーブルを解放する, PTEエントリの先に物理ページが割り当てられていた場合は,
    対象ページのアンマップを通知する
    @param[in]  as        ページテーブルの仮想空間
    @param[in]  pte_addr  解放するPTEテーブルのアドレス
 */
static void
free_user_pte_tbl(vm *as, uintptr_t pte_addr) {
	int                         i;
	pte                       ent;
	uintptr_t           page_addr;
	pte_tbl             *user_pte;

	kassert( as != NULL );
	kassert( as->pgtbl != NULL);

	user_pte = (pte_tbl *)pte_addr;
	for(i = 0; i < PGTBL_ENTRY_MAX; ++i) {

		ent = user_pte->entries[i];
		if ( page_present(ent) ) {

			user_pte->entries[i] = 0;
			invalidate_tlb();

			page_addr = get_ent_addr(ent);
			dec_page_map_count((void *)PHY_TO_KERN_STRAIGHT(page_addr));
		}
	}

	free_page((void *)pte_addr);
}

/** PDIRテーブルを解放する
    @param[in]  as         ページテーブルを解放する仮想空間
    @param[in]  pdir_addr  解放するPDIRテーブルのアドレス
 */
static void
free_user_pdir_tbl(vm *as, uintptr_t pdir_addr) {
	int                         i;
	pdire                     ent;
	uintptr_t            pte_addr;
	pdir_tbl           *user_pdir;

	kassert( as != NULL );
	kassert( as->pgtbl != NULL);

	user_pdir = (pdir_tbl *)pdir_addr;
	for(i = 0; i < PGTBL_ENTRY_MAX; ++i) {

		ent = user_pdir->entries[i];
		if ( pte_present(ent) ) {

			user_pdir->entries[i] = 0;
			pte_addr = PHY_TO_KERN_STRAIGHT(get_ent_addr(ent));
			free_user_pte_tbl(as, pte_addr);
		}
	}
	free_page((void *)pdir_addr);
}

/** PDPテーブルを解放する
    @param[in]  p         ページテーブルを解放するプロセス
    @param[in]  pdp_addr  解放するPDPテーブルのアドレス
 */
static void
free_user_pdp_tbl(vm *as, uintptr_t pdp_addr) {
	int                         i;
	pdpe                      ent;
	uintptr_t           pdir_addr;
	pdp_tbl             *user_pdp;

	kassert( as != NULL );
	kassert( as->pgtbl != NULL);

	user_pdp = (pdp_tbl *)pdp_addr;
	for(i = 0; i < PGTBL_ENTRY_MAX; ++i) {

		ent = user_pdp->entries[i];
		if ( pdir_present(ent) ) {

			user_pdp->entries[i] = 0;
			pdir_addr = PHY_TO_KERN_STRAIGHT(get_ent_addr(ent));
			free_user_pdir_tbl(as, pdir_addr);
		}
	}
	free_page((void *)pdp_addr);
}

/** PDPテーブルを割り当てる
    @param[in]  pml4      PML4テーブルのカーネル仮想アドレス
    @param[in]  vaddr     割り当て対象のユーザ仮想アドレス
    @param[out] vpdptblp  割り当てたPDIRテーブルのカーネル仮想アドレス
    @retval  0       割当て成功
    @retval -ENOMEM  ページテーブル用にメモリがない
 */
int 
alloc_pdp_tbl(pml4_tbl *pml4, uintptr_t vaddr, uintptr_t *vpdptblp) {
	int          rc;
	pml4e       ent;
	uintptr_t paddr;
	void   *new_tbl;	

	ent = get_pml4_ent(pml4, vaddr);
	if ( !pte_present(ent) ) {

		rc = get_free_page(&new_tbl);
		if ( rc != 0 )
			goto error_out;

		memset(new_tbl, 0, PAGE_SIZE);

		paddr = KERN_STRAIGHT_TO_PHY(new_tbl);
		set_pml4_ent(pml4, vaddr, 
		    paddr | PAGE_PGTBLBITS | PAGE_USER);

		ent = get_pml4_ent(pml4, vaddr);
	}

	paddr = get_ent_addr(ent);  /*  PTEテーブルの物理アドレス  */

	*vpdptblp = PHY_TO_KERN_STRAIGHT(paddr);

	rc = 0;

error_out:
	return rc;
}

/** PDIRテーブルを割り当てる
    @param[in]  pdp       PDPテーブルのカーネル仮想アドレス
    @param[in]  vaddr     割り当て対象のユーザ仮想アドレス
    @param[out] vpdirtblp 割り当てたPDIRテーブルのカーネル仮想アドレス
    @retval  0       割当て成功
    @retval -ENOMEM  ページテーブル用にメモリがない
 */
int 
alloc_pdir_tbl(pdp_tbl *pdp, uintptr_t vaddr, uintptr_t *vpdirtblp) {
	int          rc;
	pdpe       ent;
	uintptr_t paddr;
	void   *new_tbl;	

	ent = get_pdp_ent(pdp, vaddr);
	if ( !pte_present(ent) ) {

		rc = get_free_page(&new_tbl);
		if ( rc != 0 )
			goto error_out;

		memset(new_tbl, 0, PAGE_SIZE);

		paddr = KERN_STRAIGHT_TO_PHY(new_tbl);
		set_pdp_ent(pdp, vaddr, 
		    paddr | PAGE_PGTBLBITS | PAGE_USER);
		ent = get_pdp_ent(pdp, vaddr);
	}

	paddr = get_ent_addr(ent);  /*  PTEテーブルの物理アドレス  */

	*vpdirtblp = PHY_TO_KERN_STRAIGHT(paddr);

	rc = 0;

error_out:
	return rc;
}

/** PTEテーブルを割り当てる
    @param[in]  pdir     ページディレクトリテーブルのカーネル仮想アドレス
    @param[in] vaddr     割り当て対象のユーザ仮想アドレス
    @param[out] vptetblp 割り当てたPTEテーブルのカーネル仮想アドレス
    @retval  0       割当て成功
    @retval -ENOMEM  ページテーブル用にメモリがない
 */
int 
alloc_pte_tbl(pdir_tbl *pdir, uintptr_t vaddr, uintptr_t *vptetblp) {
	int          rc;
	pdire       ent;
	uintptr_t paddr;
	void   *new_tbl;	

	ent = get_pdir_ent(pdir, vaddr);
	if ( !pte_present(ent) ) {

		rc = get_free_page(&new_tbl);
		if ( rc != 0 )
			goto error_out;

		memset(new_tbl, 0, PAGE_SIZE);

		paddr = KERN_STRAIGHT_TO_PHY(new_tbl);
		set_pdir_ent(pdir, vaddr, 
		    paddr | PAGE_PGTBLBITS | PAGE_USER);
		ent = get_pdir_ent(pdir, vaddr);
	}

	paddr = get_ent_addr(ent);  /*  PTEテーブルの物理アドレス  */

	*vptetblp = PHY_TO_KERN_STRAIGHT(paddr);

	rc = 0;

error_out:
	return rc;
}

/** VMAの保護属性からページプロテクションビットを生成する
    @param[in] prot VMA保護属性
    @return    ページプロテクションビット
 */
static uint64_t
vma_prot_to_page_flags(vma_prot prot) {
	uint64_t v;

	v = (PAGE_PRESENT | PAGE_USER);

	if ( prot == VMA_PROT_NONE )
		return 0;
	
	if ( prot & VMA_PROT_W) 
		v |= PAGE_WRITABLE;

	if ( !( prot & VMA_PROT_X) )
		v |= PAGE_EXECUTE_DISABLE;
	
	return v;
}

/** ページプロテクションビットからVMAの保護属性を生成する
    @param[in] prot_bits ページプロテクションビット
    @return  VMAの保護属性
 */
static vma_prot 
page_flags_to_vma_prot(uint64_t prot_bits) {
	vma_prot v;

	if ( !( prot_bits & PAGE_PRESENT ) )
		return VMA_PROT_NONE;

	v = VMA_PROT_R;
	if ( prot_bits & PAGE_WRITABLE )
		v |= VMA_PROT_W;

	if ( !( prot_bits & PAGE_EXECUTE_DISABLE ) )
		v |= VMA_PROT_X;
	
	return v;
}

/** ユーザ空間に物理ページをマップする
    @param[in] as    マップ先のアドレス空間
    @param[in] vaddr 仮想アドレス
    @param[in] paddr 物理アドレス
    @param[in] prot  アクセス属性
    @retval 0 正常にマップした
    @retval 非0 ページテーブルの割当てに失敗した
 */
int
hal_map_user_page(vm *as, uintptr_t vaddr, uintptr_t kpaddr, vma_prot prot) {
	int              rc;
	uintptr_t page_attr;
	uintptr_t  pdp_addr;
	uintptr_t pdir_addr;
	uintptr_t  pte_addr;

	kassert( as != NULL );
	kassert( mutex_locked_by_self(&as->asmtx) );
	kassert( as->pgtbl != NULL );

	page_attr = vma_prot_to_page_flags(prot);
	if ( page_attr == 0 )
		return 0;  /*  VMA_PROT_NONEの場合はマップせず正常終了する  */
	
	rc = alloc_pdp_tbl(as->pgtbl, vaddr, &pdp_addr);
	kassert( rc == 0 );

	rc = alloc_pdir_tbl((pdp_tbl *)pdp_addr, vaddr, &pdir_addr);
	kassert( rc == 0 );

	rc = alloc_pte_tbl((pdir_tbl *)pdir_addr, vaddr, &pte_addr);
	kassert( rc == 0 );

	kpaddr = PAGE_START(kpaddr);
	page_attr &= (PAGE_SIZE - 1);

	set_pte_ent((pte_tbl *)pte_addr, vaddr, KERN_STRAIGHT_TO_PHY(kpaddr) | page_attr );
	inc_page_map_count((void *)kpaddr);

	invalidate_tlb();

#if defined(DEBUG_USER_PGTBL_MAP)
	kprintf(KERN_DBG, "map_user_page pgtbl=%p vaddr=%p\n", as->pgtbl, vaddr );
	kprintf(KERN_DBG, "map_user_page PML4->PDP pdp=%p, pml4-ent=0x%lx\n",
	    KERN_STRAIGHT_TO_PHY(pdp_addr), get_pml4_ent(as->pgtbl, vaddr));
	kprintf(KERN_DBG, "map_user_page PDP->PDIR pdir=%p, pdp-ent=0x%lx\n",
	    KERN_STRAIGHT_TO_PHY(pdir_addr), get_pdp_ent((pdp_tbl *)pdp_addr, vaddr));
	kprintf(KERN_DBG, "map_user_page PDIR->PTETBL ptetbl=%p, pdir-ent=0x%lx\n",
	    KERN_STRAIGHT_TO_PHY(pte_addr), get_pdir_ent((pdir_tbl *)pdir_addr, vaddr));
	kprintf(KERN_DBG, "map_user_page PTE->PAGE page=%p, pte-ent=0x%lx\n",
	    KERN_STRAIGHT_TO_PHY(kpaddr), get_pte_ent((pte_tbl *)pte_addr, vaddr));
#endif  /*  DEBUG_USER_PGTBL_MAP  */
	
	return 0;
}

/** 仮想アドレスの仮物変換を取得する
    @param[in] as     マップ先のアドレス空間
    @param[in] vaddr  仮想アドレス
    @param[in] kvaddrp カーネルストレートマップ域でのページアドレス返却先
    @param[in] protp  アクセス属性返却域
    @retval  0       正常に取得した
    @retval -ENOENT  対象の仮想アドレスにページが割り当てられていない。
 */
int
hal_translate_user_page(vm *as, uintptr_t vaddr, uintptr_t *kvaddrp, vma_prot *protp) {
	pml4e            pml4_ent;
	pdpe              pdp_ent;
	pdire            pdir_ent;
	pte               pte_ent;

	kassert( as != NULL );
	kassert( mutex_locked_by_self(&as->asmtx) );
	kassert( as->pgtbl != NULL );
	kassert( protp != NULL );

	pml4_ent = get_pml4_ent(as->pgtbl, vaddr);
	if ( !pdp_present(pml4_ent) )
		return -ENOENT;

	pdp_ent = get_pdp_ent((pdp_tbl *)PHY_TO_KERN_STRAIGHT(get_ent_addr(pml4_ent)), 
	    vaddr);
	if ( !pdir_present(pdp_ent) )
		return -ENOENT;

	pdir_ent = get_pdir_ent((pdir_tbl *)PHY_TO_KERN_STRAIGHT(get_ent_addr(pdp_ent)), 
	    vaddr);
	if ( !pte_present(pdir_ent) ) 
		return -ENOENT;

	pte_ent = get_pte_ent((pte_tbl *)PHY_TO_KERN_STRAIGHT(get_ent_addr(pdir_ent)), 
	    vaddr);
	if ( !pte_present(pte_ent) ) 
		return -ENOENT;

	*protp = page_flags_to_vma_prot( get_ent_attr( pte_ent ) );

	*kvaddrp = PHY_TO_KERN_STRAIGHT(get_ent_addr(pte_ent));

	return 0;
}

/** ユーザ空間から指定した仮想アドレスの仮物変換を解除する
    @param[in] as マップ先のアドレス空間
    @param[in] vaddr 仮想アドレス
    @retval 0 正常にアンマップした
 */
int
hal_unmap_user_page(vm *as, uintptr_t vaddr) {
	uintptr_t           paddr;
	pml4e            pml4_ent;
	pdpe              pdp_ent;
	pdire            pdir_ent;
	pte               pte_ent;

	kassert( mutex_locked_by_self(&as->asmtx) );
	kassert( as->pgtbl != NULL );
	
	pml4_ent = get_pml4_ent(as->pgtbl, vaddr);
	if ( !pdp_present(pml4_ent) )
		goto error_out;

	pdp_ent = get_pdp_ent((pdp_tbl *)PHY_TO_KERN_STRAIGHT(get_ent_addr(pml4_ent)), 
	    vaddr);
	if ( !pdir_present(pdp_ent) )
		goto error_out;

	pdir_ent = get_pdir_ent((pdir_tbl *)PHY_TO_KERN_STRAIGHT(get_ent_addr(pdp_ent)), 
	    vaddr);
	if ( !pte_present(pdir_ent) ) 
		goto error_out;

	pte_ent = get_pte_ent((pte_tbl *)PHY_TO_KERN_STRAIGHT(get_ent_addr(pdir_ent)), 
	    vaddr);
	if ( !pte_present(pte_ent) ) 
		goto error_out;

        /* 
	 * ページ解放前にページテーブルを無効化し, 
	 * 解放したページを破壊しないようにする
	 */
	paddr = get_ent_addr(pte_ent);
	set_pte_ent((pte_tbl *)PHY_TO_KERN_STRAIGHT(get_ent_addr(pdir_ent)), vaddr, 0);
	invalidate_tlb();  

	/*
	 * マップカウントを減算  
	 * マップしているプロセスが居なければページアロケータがページを解放する。
	 */
	dec_page_map_count((void *)PHY_TO_KERN_STRAIGHT(paddr)); 
	
#if defined(DEBUG_USER_PGTBL_MAP)
	kprintf(KERN_DBG, "unmap_user_page pgtbl=%p vaddr=%p\n", as->pgtbl, vaddr );
	kprintf(KERN_DBG, "unmap_user_page PML4->PDP pdp=%p, pml4-ent=0x%lx\n",
	    get_ent_addr(pml4_ent), get_pml4_ent(as->pgtbl, vaddr));
	kprintf(KERN_DBG, "unmap_user_page PDP->PDIR pdir=%p, pdp-ent=0x%lx\n",
	    get_ent_addr(pdp_ent), pdp_ent);
	kprintf(KERN_DBG, "unmap_user_page PDIR->PTETBL ptetbl=%p, pdir-ent=0x%lx\n",
	    get_ent_addr(pdir_ent), pdir_ent);
	kprintf(KERN_DBG, "unmap_user_page PTE->PAGE page=%p, pte-ent=0x%lx\n",
	    paddr, pte_ent);
#endif  /*  DEBUG_USER_PGTBL_MAP  */

	return 0;

error_out:
#if defined(DEBUG_USER_PGTBL_MAP)
	kprintf(KERN_DBG, "unmap_user_pageg pgtbl=%p vaddr=%p not mapped.\n", 
	    as->pgtbl, vaddr);
#endif  /*  DEBUG_USER_PGTBL_MAP  */
	return -ENOENT;
}

/** プロセスのページテーブルを解放する
    @param[in] as マップ先のアドレス空間
 */
void
hal_free_user_page_table(vm *as) {
	int                         i;
	pml4e                     ent;
	uintptr_t            pdp_addr;
	pml4_tbl           *user_pml4;

	kassert( as != NULL );
	kassert( mutex_locked_by_self(&as->asmtx) );

	if ( as->pgtbl == NULL ) 
		return;

	user_pml4 = (pml4_tbl *)as->pgtbl;
	for(i = 0; i < (int)PML4_INDEX(KERN_VMA_BASE); ++i) {
		
		ent = user_pml4->entries[i];
		if ( pdp_present(ent) ) {

			user_pml4->entries[i] = 0;
			pdp_addr = PHY_TO_KERN_STRAIGHT(get_ent_addr(ent));
			free_user_pdp_tbl(as, pdp_addr);
		}
	}
	free_page((void *)as->pgtbl);
	as->pgtbl = NULL;
}

/** プロセスのページテーブルを確保する
    @param[in] as 仮想アドレス空間
    @note カーネル領域以降の仮想アドレスは, カーネルのページテーブルの
    内容とおなじテーブルを参照するようにPML4テーブルを構成する.
 */
int
hal_alloc_user_page_table(vm *as) {
	int                                rc;
	uintptr_t                   pml4_addr;
	void                        *tmp_addr;
	pml4_tbl                   *kern_pml4;
	pml4_tbl                   *user_pml4;
	int                                 i;
	proc *kproc = hal_refer_kernel_proc();
	vm                           *kern_as;

	kassert( as != NULL );

	kern_as = &kproc->vm;
	kassert( kern_as->pgtbl != NULL );	

	rc = get_free_page(&tmp_addr);
	if ( rc != 0 )
		goto error_out;

	memset((void *)tmp_addr, 0, PAGE_SIZE);
	pml4_addr = (uintptr_t)tmp_addr;

	/*
	 * Copy Kernel Area
	 */
	for(i = PML4_INDEX(KERN_VMA_BASE), 
			    kern_pml4 = (pml4_tbl *)kern_as->pgtbl,
			    user_pml4 = (pml4_tbl *)pml4_addr;
		    i < PGTBL_ENTRY_MAX; ++i) {

			user_pml4->entries[i] = kern_pml4->entries[i];
		}

	as->pgtbl = (pgtbl_t *)pml4_addr;

error_out:
	return rc;
}

