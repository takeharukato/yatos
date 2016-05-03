/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Thread scheduler relevant definitions                             */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_SCHED_H)
#define  _KERN_SCHED_H 

#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>

#include <kern/thread.h>

void sched_schedule(void);
void sched_init_subsys(void);
#endif  /*  _KERN_SCHED_H   */
