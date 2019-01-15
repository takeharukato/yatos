/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  ELF format relevant definitions                                   */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_ELF_H)
#define _KERN_ELF_H

#include <stdint.h>
#include <stddef.h>

#include <kern/elf-common.h>
#include <hal/machine-elf.h>

#if  __ELF_WORD_SIZE == 64
#include <kern/elf64.h>
#elif __ELF_WORD_SIZE == 32 
#include <kern/elf32.h>
#else  /*  __ELF_WORD_SIZE == 32  */
#error "__ELF_WORD_SIZE must be defined as 32 or 64"
#endif  /*   __ELF_WORD_SIZE == 64  */


#include <kern/elf-generic.h>

#endif  /*  _KERN_ELF_H  */
