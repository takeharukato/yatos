/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  ELF loader relevant definitions                                   */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_ELFLDR_H)
#define  _KERN_ELFLDR_H 

#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/elf.h>
int setup_process_image_from_ELF_on_memory(proc *_proc, void *_m);
#endif  /*  _KERN_ELFLDR_H   */
