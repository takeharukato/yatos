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

void map_kernel_page(uintptr_t _paddr, uintptr_t _vaddr, 
    uintptr_t _page_attr, void *_kpgtbl, int (*_get_page)(void **addrp));

static int
alloc_boot_kmap_page(void **addrp) {
	karch_info *info;

	kassert( addrp != NULL );

	info = _refer_boot_info();

	info->boot_kpgtbl_start -= PAGE_SIZE;

	*addrp = (void *)PHY_TO_KERN_STRAIGHT( info->boot_kpgtbl_start );
	
	return 0;
}

void
x86_64_boot_map_kernel(karch_info *info) {
	uintptr_t  phy_last_mem;
	uintptr_t      cur_page;
	uintptr_t     cur_vaddr;
	uintptr_t    start_addr;
	uintptr_t     last_addr;

	info->boot_kpgtbl_start = KERN_KPGTBL_MAX;
	info->boot_kpgtbl = info->kpgtbl = (void *)PHY_TO_KERN_STRAIGHT(KERN_KPGTBL_MAX);
	memset((void *)info->kpgtbl, 0, PAGE_SIZE);

	start_addr = KERN_STRAIGHT_PAGE_START(info->phy_mem_min);
	phy_last_mem = ( ( info->phy_mem_max >  ( info->mem_upper_kb * 1024UL ) ) ?
	    ( info->mem_upper_kb * 1024 ) : info->phy_mem_max ) - 1;
	last_addr = KERN_STRAIGHT_PAGE_END( phy_last_mem );

	/*
	 * kernel straight map
	 */
	kprintf(KERN_INF, "boot-map-kernel : [%p, %p] to %p\n",
	    (void *)start_addr, (void *)last_addr, KERN_VMA_BASE);


	for( cur_page = start_addr, cur_vaddr = KERN_VMA_BASE;
	     cur_page <= last_addr;
	     cur_page += KERN_STRAIGHT_PAGE_SIZE, cur_vaddr += KERN_STRAIGHT_PAGE_SIZE) {

		map_kernel_page( cur_page,  cur_vaddr, PAGE_WRITABLE, 
		    info->kpgtbl, alloc_boot_kmap_page);
	}
#if defined(SHOW_BOOT_PGTBL_MAP)
	kprintf(KERN_INF, "boot-map-kernel page table :(min, max, phy)=(%p, %p, %p)\n",
	    (void *)info->boot_kpgtbl_start, info->kpgtbl, (uintptr_t)KERN_STRAIGHT_TO_PHY(info->kpgtbl));
#endif  /*  SHOW_BOOT_PGTBL_MAP  */
	load_pgtbl((uintptr_t)KERN_STRAIGHT_TO_PHY(info->kpgtbl));
}
