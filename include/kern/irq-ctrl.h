/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  IRQ control relevant definitions                                  */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_IRQ_CTRL_H)
#define  _KERN_IRQ_CTRL_H 

#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>

#include <hal/pic.h>

typedef struct _irq_cntlr{
	int (*enable)(intr_no  _irq);
	int (*disable)(intr_no _irq);
	int (*send_eoi)(intr_no _irq);
}irq_cntlr;
#define __IRQ_CTRL_INITIALIZER(_enable, _disable, _eoi)	\
	{				\
	.enable  = _enable,		\
	.disable = _disable,            \
	.send_eoi = _eoi,               \
	}

void hal_init_pic(void);
#endif  /*  _KERN_IRQ_CTRL_H   */
