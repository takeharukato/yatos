/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Intel 8254 Programmable Interval Timer(PIT) relevant routines     */
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

#include <hal/portio.h>
#include <hal/i8254.h>

void
hal_timer_init(void) {
	uint32_t  divisor;

	divisor = I8254_INPFREQ / HZ;
	 
	/* Square Wave(方形波ジェネレータ)モード, バイナリカウンタ, 16bitカウンタに設定  */
	out_port_byte(I8254_PORT_MODECNTL, I8254_CMD_INTERVAL_TIMER);
	/* 下位バイト, 上位バイトの順に,  チャネル0に周波数を設定  */
	out_port_byte(I8254_PORT_CHANNEL0, (uint8_t)( divisor & ~( (uint8_t)(0) ) ) );
	out_port_byte(I8254_PORT_CHANNEL0, (uint8_t)( (divisor >> 8) & ~( (uint8_t)(0) ) ) );
}
