/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  architecture dependent page definitions                           */
/*                                                                    */
/**********************************************************************/
#if !defined(__HAL_ARCH_PAGE_H)
#define __HAL_ARCH_PAGE_H

#define PAGE_SHIFT                      (12)         /*  4KiB  */
#define PAGE_SIZE                   (0x1000)         /*  4KiB  */
#define KERN_STRAIGHT_PAGE_SIZE   (0x200000)         /*  2MiB  */

#if !defined(ASM_FILE)
#include <stddef.h>
#include <stdint.h>

#define PAGE_ALIGNED(addr)						\
	( ( (uintptr_t)( ( (uintptr_t)(addr) ) &			\
		( (uintptr_t)(PAGE_SIZE) - (uintptr_t)1 ) ) ) == 0 )

#define PAGE_START(addr)					\
	( (uintptr_t)( ( (uintptr_t)(addr) ) &			\
	    ~( (uintptr_t)(PAGE_SIZE) - (uintptr_t)1 ) ) )

#define PAGE_NEXT(addr)				\
	( (uintptr_t)( PAGE_START( (addr) ) +	\
	(uintptr_t)PAGE_SIZE ) )

#define PAGE_END(addr)					\
	( (uintptr_t)( PAGE_NEXT( (addr) ) - 1 ) )

#define KERN_STRAIGHT_PAGE_ALIGNED(addr)						\
	( ( (uintptr_t)( ( (uintptr_t)(addr) ) &			\
		( (uintptr_t)(KERN_STRAIGHT_PAGE_SIZE) - (uintptr_t)1 ) ) ) == 0 )

#define KERN_STRAIGHT_PAGE_START(addr)					\
	( (uintptr_t)( ( (uintptr_t)(addr) ) &				\
	    ~( (uintptr_t)(KERN_STRAIGHT_PAGE_SIZE) - (uintptr_t)1 ) ) )

#define KERN_STRAIGHT_PAGE_NEXT(addr)			  \
	( (uintptr_t)KERN_STRAIGHT_PAGE_START( (addr) ) + \
	    (uintptr_t)(KERN_STRAIGHT_PAGE_SIZE) )

#define KERN_STRAIGHT_PAGE_END(addr)		\
	( KERN_STRAIGHT_PAGE_NEXT((addr)) - 1 )

#endif  /*  !ASM_FILE  */

#endif  /*  __HAL_ARCH_PAGE_H  */
