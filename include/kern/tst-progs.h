/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  relevant definitions                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_TST_PROGS_H)
#define  _KERN_TST_PROGS_H 

#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>

void _setup_test_progs(void);

extern void proc_create_test(void);
extern void thread_test(void);
extern void test_memmove(void);
extern void timer_test(void);
extern void lpc1_test(void);
extern void lpc2_test(void);
extern void kserv_test(void);
extern void wait_test(void);
#endif  /*  _KERN_TST_PROGS_H   */
