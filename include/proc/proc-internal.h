/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Process module internal definition                                */
/*  Note: Following definitions must be used in a logical module only */
/*                                                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(__PROC_INTERNAL_H)
#define  __PROC_INTERNAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>

struct _proc;
int _proc_load_ELF_from_memory(struct _proc *_proc, void *_kvaddr);
void _proc_common_init(struct _proc *p);
#endif  /*  __PROC_INTERNAL_H   */
