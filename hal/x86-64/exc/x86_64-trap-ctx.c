/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  X86-64 trap context routines                                      */
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

#include <hal/segment.h>

/** ユーザからの割込み/トラップであることを確認する
    @param[in] ctx 割込み/例外コンテキスト
    @retval true   ユーザからの割込み/トラップである
    @retval false  ユーザからの割込み/トラップでない
 */
bool
hal_is_intr_from_user(void *ctx) {
	
	kassert( ctx != NULL );
	
	return ( ((trap_context *)ctx)->cs == GDT_USER_CODE64 );
}
