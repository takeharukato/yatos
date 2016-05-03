/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual module internal definitions                               */
/*  Note: Following definitions must be used in a logical module only */
/*                                                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(__VM_INTERNAL_H)
#define  __VM_INTERNAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>

struct _vm;
struct _vma;
int _vm_find_vma_nolock(struct _vm *as, void *vaddr, struct _vma **res);
#endif  /*  __VM_INTERNAL_H   */
