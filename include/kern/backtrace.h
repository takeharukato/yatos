/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  backtrace relevant definitions                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_BACKTRACE_H)
#define  _KERN_BACKTRACE_H 
#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>

#include <hal/stack-ops.h>

void print_back_trace(void *_basep);
void hal_back_trace(int (*_trace_out)(int depth, uintptr_t *_bpref, void *_caller, 
	void *_next_bp, void *_argp), void *_basep, void *argp);
#endif  /*  _KERN_BACKTRACE_H   */
