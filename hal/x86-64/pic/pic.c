/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Programmable Interrupt Controller(PIC) routines                   */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/errno.h>
#include <kern/spinlock.h>
#include <kern/irq.h>
#include <kern/timer.h>

/** 割込みマスクを更新する
    @param[in] msk 更新するマスクパターン
 */
void
hal_pic_update_irq_mask(intr_mask_state msk){

	i8259_update_irq_mask(msk);
}

/** 割込みコントローラを初期化する
 */
void
hal_init_pic(void) {
	int rc;

	init_i8259_pic();

	rc = kcom_tim_register_timer_irq(0, NULL);
	kassert( rc == 0 );
}

