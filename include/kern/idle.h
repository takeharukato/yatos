/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Idle thread relevant definitions                                  */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_IDLE_H)
#define  _KERN_IDLE_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>

struct _thread;
struct _thread *idle_refer_idle_thread(void);
void idle_start(void);
void idle_init_current_cpu_idle(void);
void idle_init_subsys(void);

void hal_idle(void);
#endif  /*  _KERN_IDLE_H   */
