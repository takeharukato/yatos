/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  kernel address map initialization                                 */
/*                                                                    */
/**********************************************************************/
#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/page.h>

#include <hal/kernlayout.h>
#include <hal/arch-page.h>
#include <hal/segment.h>
#include <hal/pgtbl.h>
#include <hal/prepare.h>

//#define SHOW_BOOT_PGTBL_MAP

karch_info  *_refer_boot_info(void) ;

void
map_kernel_page(uintptr_t paddr, uintptr_t vaddr, 
    uintptr_t page_attr, void *kpgtbl, int (*_get_page)(void **addrp)) {
	int             rc;
	pml4e          ent1;
	pdpe           ent2;
	pdire          ent3;
	uintptr_t  pdp_addr;
	uintptr_t pdir_addr;
	void      *new_page;

	kassert( _get_page != NULL );

	ent1 = get_pml4_ent((pgtbl_t *)kpgtbl, vaddr);
	if ( !pdp_present(ent1) ) {

		rc = _get_page( &new_page );
		kassert( rc == 0 );

		pdp_addr = (uintptr_t)new_page;
		kassert((void *)pdp_addr != NULL);
		memset((void *)pdp_addr, 0, PAGE_SIZE);

		pdp_addr = KERN_STRAIGHT_TO_PHY(pdp_addr);
		set_pml4_ent((pgtbl_t *)kpgtbl, vaddr, pdp_addr | PAGE_PRESENT);
		ent1 = get_pml4_ent((pgtbl_t *)kpgtbl, vaddr);
	} else {
		
		pdp_addr = get_ent_addr(ent1);
	}

#if defined(DEBUG_SHOW_KERNEL_MAP)
	kprintf(KERN_DBG, "boot_map_kernel_page PML4->PDP (%p, %p):pdp=%p, ent=0x%lx\n",
	    paddr, vaddr, pdp_addr, ent1);
#endif  /*  DEBUG_SHOW_KERNEL_MAP  */

	pdp_addr = PHY_TO_KERN_STRAIGHT(pdp_addr);
	ent2 = get_pdp_ent((pdp_tbl *)pdp_addr, vaddr);
	if (!pdir_present(ent2)) {

		rc = _get_page( &new_page );
		kassert( rc == 0 );

		pdir_addr = (uintptr_t)new_page;
		kassert((void *)pdir_addr != NULL);
		memset((void *)pdir_addr, 0, PAGE_SIZE);

		pdir_addr = KERN_STRAIGHT_TO_PHY(pdir_addr);
		set_pdp_ent((pdp_tbl *)pdp_addr, vaddr, pdir_addr | PAGE_PRESENT);
		ent2 = get_pdp_ent((pdp_tbl *)pdp_addr, vaddr);
	} else {
		
		pdir_addr = get_ent_addr(ent2);
	}

#if defined(DEBUG_SHOW_KERNEL_MAP)
	kprintf(KERN_DBG, "boot_map_kernel_page PDP->PDIR (%p, %p):pdir=%p, ent=0x%lx\n",
	    paddr, vaddr, pdir_addr, ent2);
#endif  /*  DEBUG_SHOW_KERNEL_MAP  */

	pdir_addr = PHY_TO_KERN_STRAIGHT(pdir_addr);
	ent3 = get_pdir_ent((pdir_tbl *)pdir_addr, vaddr);
	kassert(!pdir_present(ent3));
	paddr = KERN_STRAIGHT_PAGE_START(paddr);
	page_attr &= (PAGE_SIZE - 1);
	set_pdir_ent((pdir_tbl *)pdir_addr, vaddr, 
	    paddr | page_attr | PAGE_PRESENT | PAGE_2MB );
#if defined(DEBUG_SHOW_KERNEL_MAP)
	kprintf(KERN_DBG, "boot_map_kernel_page PDIR->PAGE (%p, %p):paddr=%p, ent=0x%lx\n",
	    paddr, vaddr, paddr, get_pdir_ent((pdir_tbl *)pdir_addr, vaddr));
#endif  /*  DEBUG_SHOW_KERNEL_MAP  */
}

static void 
map_high_io_area(void *kpgtbl) {
	uintptr_t  cur_page;
	uintptr_t cur_vaddr;
	uintptr_t last_addr;	

	/*
	 * High I/O memory
	 */
	kprintf(KERN_DBG, "High APIC I/O region : [%p, %p] to %p\n",
	    (void *)KERN_HIGH_IO_AREA,
	    (void *)( KERN_HIGH_IO_AREA + KERN_HIGH_IO_SIZE ),
	    (void *)KERN_HIGH_IO_BASE );

	last_addr = KERN_STRAIGHT_PAGE_END( KERN_HIGH_IO_AREA + 
	    KERN_HIGH_IO_SIZE );

	for( cur_page = KERN_STRAIGHT_PAGE_START( KERN_HIGH_IO_AREA ), 
		     cur_vaddr = KERN_HIGH_IO_BASE;
	     cur_page <= last_addr;
	     cur_page += KERN_STRAIGHT_PAGE_SIZE, 
		     cur_vaddr += KERN_STRAIGHT_PAGE_SIZE) {

		map_kernel_page( cur_page,  cur_vaddr, PAGE_WRITABLE | PAGE_NONCACHABLE, 
		    kpgtbl, get_free_page);
	}
}

void
x86_64_remap_kernel(karch_info *info){
	int               rc;
	uintptr_t   cur_page;
	uintptr_t  cur_vaddr;
	uintptr_t start_addr;
	uintptr_t  last_addr;

	kassert( info != NULL );

	rc = get_free_page( &info->kpgtbl );
	kassert( rc == 0 );

	memset(info->kpgtbl, 0, PAGE_SIZE);

	/*
	 * Remap all physical memory area
	 */

	start_addr = KERN_STRAIGHT_PAGE_START( info->phy_mem_min );
	last_addr = ( KERN_STRAIGHT_PAGE_ALIGNED( info->phy_mem_max ) ?
	    ( info->phy_mem_max ) : KERN_STRAIGHT_PAGE_NEXT( info->phy_mem_max ) ) - 1;

	kprintf(KERN_INF, "map-kernel : [%p, %p] to %p\n",
	    (void *)start_addr, (void *)last_addr, KERN_VMA_BASE);

	for( cur_page = start_addr, cur_vaddr = KERN_VMA_BASE;
	     cur_page <= last_addr;
	     cur_page += KERN_STRAIGHT_PAGE_SIZE, cur_vaddr += KERN_STRAIGHT_PAGE_SIZE) {

		/*
		 * カーネル領域はグローバルビット(TLBフラッシュの対象外)とする
		 */
		if ( ( KERN_STRAIGHT_PAGE_START( (uintptr_t)&_kernel_start ) <= cur_page ) &&
		    ( cur_page < KERN_STRAIGHT_PAGE_END( (uintptr_t)&_kernel_end ) ) ) 
			map_kernel_page( cur_page,  cur_vaddr, PAGE_WRITABLE | PAGE_GLOBAL, 
			    info->kpgtbl, get_free_page);
		else 
			map_kernel_page( cur_page,  cur_vaddr, PAGE_WRITABLE, 
			    info->kpgtbl, get_free_page);
	} 

	map_high_io_area( info->kpgtbl );
	load_pgtbl((uintptr_t)KERN_STRAIGHT_TO_PHY(info->kpgtbl));
}
