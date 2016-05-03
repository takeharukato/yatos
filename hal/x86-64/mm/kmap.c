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

#include <hal/kernlayout.h>
#include <hal/arch-page.h>
#include <hal/segment.h>
#include <hal/pgtbl.h>
#include <hal/prepare.h>

//#define SHOW_BOOT_PGTBL_MAP

static uintptr_t 
alloc_kmap_page(karch_info *info) {

	info->kpgtbl_start -= PAGE_SIZE;
	return info->kpgtbl_start;
}

static void
boot_map_kernel_page(uintptr_t paddr, uintptr_t vaddr, 
    karch_info *info, uintptr_t page_attr) {
	pml4e  ent1;
	pdpe   ent2;
	pdire  ent3;
	uintptr_t pdp_addr;
	uintptr_t pdir_addr;

	ent1 = get_pml4_ent(info->kpgtbl, vaddr);
	if ( !pdp_present(ent1) ) {

		pdp_addr = PHY_TO_KERN_STRAIGHT(alloc_kmap_page(info));
		kassert((void *)pdp_addr != NULL);
		memset((void *)pdp_addr, 0, PAGE_SIZE);

		pdp_addr = KERN_STRAIGHT_TO_PHY(pdp_addr);
		set_pml4_ent(info->kpgtbl, vaddr, pdp_addr | PAGE_PRESENT);
		ent1 = get_pml4_ent(info->kpgtbl, vaddr);
	} else {
		
		pdp_addr = get_ent_addr(ent1);
	}

#if defined(SHOW_BOOT_PGTBL_MAP)
	kprintf(KERN_DBG, "boot_map_kernel_page PML4->PDP (%p, %p):pdp=%p, ent=0x%lx\n",
	    paddr, vaddr, pdp_addr, ent1);
#endif  /*  SHOW_BOOT_PGTBL_MAP  */

	pdp_addr = PHY_TO_KERN_STRAIGHT(pdp_addr);
	ent2 = get_pdp_ent((pdp_tbl *)pdp_addr, vaddr);
	if (!pdir_present(ent2)) {

		pdir_addr = PHY_TO_KERN_STRAIGHT(alloc_kmap_page(info));
		kassert((void *)pdir_addr != NULL);
		memset((void *)pdir_addr, 0, PAGE_SIZE);

		pdir_addr = KERN_STRAIGHT_TO_PHY(pdir_addr);
		set_pdp_ent((pdp_tbl *)pdp_addr, vaddr, pdir_addr | PAGE_PRESENT);
		ent2 = get_pdp_ent((pdp_tbl *)pdp_addr, vaddr);
	} else {
		
		pdir_addr = get_ent_addr(ent2);
	}

#if defined(SHOW_BOOT_PGTBL_MAP)
	kprintf(KERN_DBG, "boot_map_kernel_page PDP->PDIR (%p, %p):pdir=%p, ent=0x%lx\n",
	    paddr, vaddr, pdir_addr, ent2);
#endif  /*  SHOW_BOOT_PGTBL_MAP  */

	pdir_addr = PHY_TO_KERN_STRAIGHT(pdir_addr);
	ent3 = get_pdir_ent((pdir_tbl *)pdir_addr, vaddr);
	kassert(!pdir_present(ent3));
	paddr = KERN_STRAIGHT_PAGE_START(paddr);
	page_attr &= (PAGE_SIZE - 1);
	set_pdir_ent((pdir_tbl *)pdir_addr, vaddr, 
	    paddr | page_attr | PAGE_PRESENT | PAGE_2MB );
#if defined(SHOW_BOOT_PGTBL_MAP)
	kprintf(KERN_DBG, "boot_map_kernel_page PDIR->PAGE (%p, %p):paddr=%p, ent=0x%lx\n",
	    paddr, vaddr, paddr, get_pdir_ent((pdir_tbl *)pdir_addr, vaddr));
#endif  /*  SHOW_BOOT_PGTBL_MAP  */
}

void
remap_kernel(karch_info *info, uintptr_t mem_max) {
	uintptr_t     last;
	uintptr_t cur_addr;

	info->kpgtbl_start = KERN_KPGTBL_MAX;
	info->kpgtbl = (void *)PHY_TO_KERN_STRAIGHT(KERN_KPGTBL_MAX);
	memset((void *)info->kpgtbl, 0, PAGE_SIZE);

	last = ( mem_max > KERN_PHY_MAX ) ? (KERN_PHY_MAX) : mem_max;

	/*
	 * kernel straight map
	 */
	kprintf(KERN_INF, "map-kernel : [%p, %p] to %p\n",
	    (void *)PAGE_START(KERN_PHY_BASE), (void *)PAGE_END(last), KERN_VMA_BASE);
	
	for(cur_addr = PAGE_START(KERN_PHY_BASE);
	    cur_addr < PAGE_END(last);
	    cur_addr += KERN_STRAIGHT_PAGE_SIZE) {

		boot_map_kernel_page((uintptr_t)cur_addr,
		    KERN_VMA_BASE + cur_addr, info, PAGE_WRITABLE);
	}
#if defined(SHOW_BOOT_PGTBL_MAP)
	kprintf(KERN_INF, "map-kernel page table :(min, max, phy)=(%p, %p, %p)\n",
	    (void *)info->kpgtbl_start, info->kpgtbl, (uintptr_t)KERN_STRAIGHT_TO_PHY(info->kpgtbl));
#endif  /*  SHOW_BOOT_PGTBL_MAP  */

	/*
	 * High I/O memory
	 */
	kprintf(KERN_DBG, "High APIC I/O region : [%p, %p] to %p\n",
	    (void *)KERN_HIGH_IO_AREA,
	    (void *)( KERN_HIGH_IO_AREA + KERN_HIGH_IO_SIZE ),
	    (void *)KERN_HIGH_IO_BASE );

	for(cur_addr = PAGE_START(KERN_HIGH_IO_AREA);
	    cur_addr < ( PAGE_START(KERN_HIGH_IO_AREA) + (uintptr_t)KERN_HIGH_IO_SIZE );
	    cur_addr += KERN_STRAIGHT_PAGE_SIZE) {

		boot_map_kernel_page((uintptr_t)cur_addr,
		    KERN_HIGH_IO_BASE + 
		    ( ( (uintptr_t)cur_addr ) - PAGE_START(KERN_HIGH_IO_AREA) ), 
		    info, PAGE_WRITABLE | PAGE_NONCACHABLE);
	}
	

	load_pgtbl((uintptr_t)KERN_STRAIGHT_TO_PHY(info->kpgtbl));
}
