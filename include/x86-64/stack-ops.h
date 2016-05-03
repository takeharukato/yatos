/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  backtrace definitions                                             */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_STACK_OPS_H)
#define  _HAL_STACK_OPS_H 
#include <stdint.h>
#include <stddef.h>

static inline void
get_base_pointer(uintptr_t *bp){

	asm volatile("mov %%rbp, %0" : "=r" (*bp));  
}

static inline void
get_stack_pointer(uintptr_t *sp){

	asm volatile("mov %%rsp, %0" : "=r" (*sp));  
}

#endif  /*  _HAL_STACK_OPS_H   */
