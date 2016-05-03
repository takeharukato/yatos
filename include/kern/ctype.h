/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  ctype relevant definitions                                        */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_CTYPE_H)
#define  _KERN_CTYPE_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>

#define isspace(c) (				\
		( (int)(c) == ' '  ) ||		\
		( (int)(c) == '\f' ) ||		\
		( (int)(c) == '\n' ) ||		\
		( (int)(c) == '\r' ) ||		\
		( (int)(c) == '\t' ) ||		\
		( (int)(c) == '\v' ) )


#endif  /*  _KERN_CTYPE_H   */
