/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Segment routines                                                  */
/*                                                                    */
/**********************************************************************/
#include <stddef.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/errno.h>
#include <kern/spinlock.h>
#include <kern/page.h>

#include <hal/kernlayout.h>
#include <hal/segment.h>
#include <hal/traps.h>

//#define SHOW_IOMAP_INIT
//#define DEBUG_SHOW_GLOBAL_DESCRIPTOR

#if defined(DEBUG_SHOW_GLOBAL_DESCRIPTOR)
static void
show_gdt_descriptor_entry(uint16_t sel, gdt_descriptor *gdtp) {
	uintptr_t addr;
	uintptr_t limit;

	kassert(gdtp != NULL);
	kassert(gdtp->avl == 0);

	addr = gdtp->base0 | ( (uintptr_t)gdtp->base1 ) << 16 | ( (uintptr_t)gdtp->base2 ) << 24;
	limit = gdtp->limit0 | ( (uintptr_t)gdtp->limit1 ) << 16;
	
	kprintf(KERN_DBG, "%d : addr=%p limit=0x%lx %s %s %s %s %s %s %s %s %s\n", 
	    sel, addr, limit, 
	    ( gdtp->access ) ? ("accessed") : ("Not accessed"),
	    ( gdtp->rw ) ? ("Read/Write") : ("ReadOnly"),
	    ( gdtp->dc ) ? ("Conformed segment") : (""),
	    ( gdtp->exec ) ? ("Executable") : ("Not Executable"),
	    ( gdtp->dpl == X86_DESC_DPL_KERNEL ) ? ("Kernel") : ("User"),
	    ( gdtp->present ) ? ("Present") : ("NotPresent"),
	    ( gdtp->mode ) ? ("Long Mode") : ("16/32bit mode"),
	    ( gdtp->size ) ? ("32bit size") : ("16bit/64bit size"),
	    ( gdtp->gran ) ? ("page granularity") : ("byte granularity"));
}

static void
show_gdt_descriptor(gdt_descriptor *gdt_base) {
	int i;

	for(i = 0; i < (GDT_TSS64_SEL + 1); ++i) {

		show_gdt_descriptor_entry(i << 3, &gdt_base[i]);
	}
}
#else
static void
show_gdt_descriptor(gdt_descriptor __attribute__ ((unused)) *gdt_base) {
}
#endif  /*  DEBUG_SHOW_GLOBAL_DESCRIPTOR  */

static void
load_global_segment(void *p, size_t size) {
	region_descriptor rd;

	rd.rd_limit = size - 1;
	rd.rd_base = (uint64_t)p;
	lgdtr(&rd, GDT_KERN_CODE64, GDT_KERN_DATA64);
}

static void
init_tss_descriptor(gdt_descriptor *gdt_ent, uintptr_t addr, uintptr_t page_end, 
    uintptr_t limit) {
	gdt_descriptor *gdtp;
	uint64_t  *high_addr;
	tss64           *tss;
	uint16_t    *iomap_p;
	unsigned int       i;

	gdtp = gdt_ent;

	kassert(gdtp != NULL);

	gdtp->base0 = addr & 0xffff;
	gdtp->base1 = ( addr >> 16 ) & 0xff;
	gdtp->base2 = ( addr >> 24 ) & 0xff;
	
	gdtp->limit0 = limit & 0xffff;
	gdtp->limit1 = ( limit >> 16 ) & 0xf;

	gdtp->access = 1;
	gdtp->rw = X86_DESC_RDONLY;
	gdtp->dc = 0;
	gdtp->exec = X86_DESC_EXEC;
	gdtp->resv0 = 0;
	gdtp->dpl = X86_DESC_DPL_USER;
	gdtp->present = 1;
	gdtp->avl = 0;
	gdtp->mode = 0;
	gdtp->size = 0;
	gdtp->gran = 0;

	/*
	 * 64bitTSSセグメントGDTの場合は, 32bit GDTの直後に32bitの
	 * 上位32bitベースアドレスが入る
	 */
	high_addr = (uint64_t *)( gdt_ent + 1 );
	*high_addr = ( (addr >> 32) & 0xffffffff );

	/* TSSの初期化, 初期化対象はI/O bitmapのみ
	 * I/O ポート命令の引数は16ビットなので最大65536ポート使用可能
	 * 65536ポート分の16ビットチャンクビットマップが必要.
	 * 65536ポート / 8bit(バイトあたりのビット数) = 8192バイト = 2ページ
	 * buddy poolが2の冪乗でページを割り当てることから, TSSページにはgdtを含めて
	 * 4ページを割り当てている
	 */
	tss = (tss64 *)addr;

#if defined(SHOW_IOMAP_INIT)
	kprintf(KERN_CRI,
	    "TSS: start %p end %p iomap:%p\n", tss, (void *)page_end, &tss->iomap);
#endif  /*  SHOW_IOMAP_INIT  */

	for(iomap_p = &tss->iomap, i = 0; 
	    ( (void *)page_end > (void *)iomap_p) && ( (1UL<<16)/(sizeof(uint16_t)*8) ) > i;
	    ++iomap_p, ++i) {

#if defined(SHOW_IOMAP_INIT)
		kprintf(KERN_DBG,
		    "TSS: iomap: %p [%d]=0xffff\n", iomap_p,
		    ((uintptr_t)iomap_p - (uintptr_t)&tss->iomap)/sizeof(uint16_t),
		    iomap_p);
#endif  /*  SHOW_IOMAP_INIT  */
		*iomap_p = ~((uint16_t)(0));
	}
	return;
}

static void
init_gdt_descriptor_table_entry(gdt_descriptor *gdtp, uintptr_t addr, uintptr_t limit,
    unsigned rw, unsigned exec, unsigned dpl, unsigned mode, 
    unsigned size, unsigned granularity) {

	kassert(gdtp != NULL);

	gdtp->base0 = addr & 0xffff;
	gdtp->base1 = ( addr >> 16 ) & 0xff;
	gdtp->base2 = ( addr >> 24 ) & 0xff;
	
	gdtp->limit0 = limit & 0xffff;
	gdtp->limit1 = ( limit >> 16 ) & 0xf;

	gdtp->access = 0;
	gdtp->rw = rw;
	gdtp->dc = 0;

	gdtp->mode = mode;
	if (exec) {

		gdtp->exec = 1;
	} else {

		gdtp->exec = 0;
	}

	gdtp->resv0 = 1;
	gdtp->dpl = dpl;
	gdtp->present = 1;
	gdtp->avl = 0;

	if ( gdtp->mode ) {

		gdtp->size = 0;
	} else {

		gdtp->size = size;
	}

	gdtp->gran = granularity;
}

void
init_segments(void *gdtp, tss64 **tssp){
	uint64_t                *gdt;
	uint64_t                addr;

	memset(gdtp, 0, sizeof(PAGE_SIZE<<X86_64_SEGMENT_CPUINFO_PAGE_ORDER));
	gdt = (uint64_t *)gdtp;
	
	addr = (uint64_t)( gdtp + X86_64_SEGMENT_CPUINFO_OFFSET );

	gdt[GDT_NULL1_SEL] = (uint64_t)0;
	gdt[GDT_NULL2_SEL] = (uint64_t)0;

	init_gdt_descriptor_table_entry((gdt_descriptor *)&gdt[GDT_KERN_CODE32_SEL], 
	    0, 0xffffffff, X86_DESC_RDWR, X86_DESC_EXEC, X86_DESC_DPL_KERNEL, 
	    X86_DESC_32BIT_MODE, X86_DESC_32BIT_SEG, X86_DESC_PAGE_SIZE);

	init_gdt_descriptor_table_entry((gdt_descriptor *)&gdt[GDT_KERN_DATA32_SEL], 
	    0, 0xffffffff, X86_DESC_RDWR, X86_DESC_NONEXEC, X86_DESC_DPL_KERNEL, 
	    X86_DESC_32BIT_MODE, X86_DESC_32BIT_SEG, X86_DESC_PAGE_SIZE);

	init_gdt_descriptor_table_entry((gdt_descriptor *)&gdt[GDT_KERN_CODE64_SEL], 
	    0, 0xffffffff, X86_DESC_RDWR, X86_DESC_EXEC, X86_DESC_DPL_KERNEL, 
	    X86_DESC_64BIT_MODE, X86_DESC_64BIT_SEG, X86_DESC_PAGE_SIZE);

	init_gdt_descriptor_table_entry((gdt_descriptor *)&gdt[GDT_KERN_DATA64_SEL], 
	    0, 0xffffffff, X86_DESC_RDWR, X86_DESC_NONEXEC, X86_DESC_DPL_KERNEL, 
	    X86_DESC_64BIT_MODE, X86_DESC_64BIT_SEG, X86_DESC_PAGE_SIZE);

	init_gdt_descriptor_table_entry((gdt_descriptor *)&gdt[GDT_USER_CODE64_SEL], 
	    0, 0xffffffff, X86_DESC_RDWR, X86_DESC_EXEC, X86_DESC_DPL_USER, 
	    X86_DESC_64BIT_MODE, X86_DESC_64BIT_SEG, X86_DESC_PAGE_SIZE);

	init_gdt_descriptor_table_entry((gdt_descriptor *)&gdt[GDT_USER_DATA64_SEL], 
	    0, 0xffffffff, X86_DESC_RDWR, X86_DESC_NONEXEC, X86_DESC_DPL_USER, 
	    X86_DESC_64BIT_MODE, X86_DESC_64BIT_SEG, X86_DESC_PAGE_SIZE);

	init_tss_descriptor((gdt_descriptor *)&gdt[GDT_TSS64_SEL], addr, 
	    (uintptr_t )(gdtp + (PAGE_SIZE<<X86_64_SEGMENT_CPUINFO_PAGE_ORDER)), sizeof(tss64) - 1);
	
	load_global_segment( (void *)gdt, (GDT_TSS64_SEL + 2) * sizeof(uint64_t));

	ltr(GDT_TSS64);

	*tssp = (tss64 *)addr;

	show_gdt_descriptor((gdt_descriptor *)&gdt[0]);
}

