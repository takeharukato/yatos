/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Read time stamp counter                                           */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RDTSC_H)
#define  _HAL_RDTSC_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>

/** タイムスタンプカウンタを読み取る
 */
static inline uint64_t 
rdtsc(void) {
	unsigned long lo, hi;

        asm( "rdtsc" : "=a" (lo), "=d" (hi) ); 

        return( lo | (hi << 32) );
}

#endif  /*  _HAL_RDTSC_H   */
