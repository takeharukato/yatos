/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x86-64 ELF definitions                                            */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_MACHINE_ELF_H)
#define  _HAL_MACHINE_ELF_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#if !defined(__ELF_WORD_SIZE)
#define __ELF_WORD_SIZE     64
#endif  /*   __ELF_WORD_SIZE  */
#endif  /*  _HAL_MACHINE_ELF_H   */
