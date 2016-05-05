/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  kernel memory layout definitions                                  */
/*                                                                    */
/**********************************************************************/
#if !defined(__HAL_KERNLAYOUT_H)
#define __HAL_KERNLAYOUT_H
#include <kern/config.h>
#include <hal/arch-page.h>
#include <hal/segment.h>

#define KERN_PHYS_LOW             (0x1000)
#define KERN_HIGH_IO_AREA         (0xFE000000)
#define KERN_HIGH_MEMORY_BASE     (0x100000000)
#define KERN_PHY_BASE             (0x0000000000000000)
#define KERN_KPGTBL_MAX           (0x9F000)
#define KERN_PHY_MAX              (0x20000000000)

#define KERN_VMA_BASE             (0xFFFF800000000000)                                 
#define KERN_HIGH_IO_BASE         (0xFFFFFFFFFC000000)
#define KERN_HIGH_IO_SIZE         (KERN_HIGH_MEMORY_BASE - KERN_HIGH_IO_AREA)
                                     
#define KERN_MAX_HIGH_IO_PAGES    (KERN_HIGH_IO_SIZE / KERN_STRAIGHT_PAGE_SIZE)

#define KERN_HEAP_BASE           \
	( KERN_STRAIGHT_PAGE_START(KERN_HIGH_IO_BASE + KERN_HIGH_IO_SIZE) + \
	    KERN_STRAIGHT_PAGE_SIZE )

#define PHY_TO_KERN_STRAIGHT(phy) ( ((uintptr_t)(phy)) + KERN_VMA_BASE)
#define KERN_STRAIGHT_TO_PHY(va)  ( ((uintptr_t)(va)) - KERN_VMA_BASE)
#define PHY_TO_PGFRAME_KVADDR(phy) ( ((uintptr_t)(phy)) + KERN_VMA_BASE)
#define HIGH_IO_TO_KERN_STRAIGHT(iaddr) \
	( ((uintptr_t)(iaddr)) + KERN_HIGH_IO_BASE - KERN_HIGH_IO_AREA)
#define KERN_STRAIGHT_TO_HIGH_IO(va)  \
	( ((uintptr_t)(va)) - KERN_HIGH_IO_BASE + KERN_HIGH_IO_AREA)

#if !defined(ASM_FILE)
#include <stddef.h>
#include <stdint.h>

extern uintptr_t _rodata_start;
extern uintptr_t _rodata_end;
extern uintptr_t _data_start;
extern uintptr_t _data_end;
extern uintptr_t _bss_start;
extern uintptr_t _bss_end;
extern uintptr_t _heap_early_start;
extern uintptr_t _heap_early_end;

extern uintptr_t _kernel_start;
extern uintptr_t _kernel_end;
extern uintptr_t bsp_stack;
extern uintptr_t gdt;

#endif  /*  !ASM_FILE  */
#endif  /*  __HAL_KERNLAYOUT_H  */
