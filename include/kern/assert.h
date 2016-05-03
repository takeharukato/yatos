/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  assertion relevant definitions                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_ASSERT_H)
#define  _KERN_ASSERT_H 
#include <kern/config.h>
#include <kern/kprintf.h>

#define kassert(cond) do {						\
	if ( !(cond) ) {                                                \
		kprintf(KERN_CRI, "Assertion : [file:%s func %s line:%d ]\n", \
		    __FILE__, __func__, __LINE__);			\
		while(1);                                               \
	}								\
	}while(0)

#endif  /*  _KERN_ASSERT_H   */
