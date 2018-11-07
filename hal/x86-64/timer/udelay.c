/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  udelay relevant  routines                                         */
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
#include <kern/thread-info.h>

#include <hal/rtc.h>
#include <hal/portio.h>
#include <hal/rdtsc.h>

//#define DEBUG_UDELAY_CALIBRATION

extern void _x86_64_set_tsc_per_us(uint64_t _tsc);
extern uint64_t _x86_64_get_tsc_per_us(void);

/** udelayを行うためにマイクロ秒毎にCPUのタイムスタンプカウンタがいくつ進むか確認する
 */
static void 
x86_64_calcluate_tsc_per_sec(void) {
	volatile uint8_t      val;
	volatile uint64_t  t1, t2;
	uint64_t       tsc_per_us;

#if defined(DEBUG_UDELAY_CALIBRATION)
	kprintf(KERN_DBG, "CPU[%d] udelay calibration with RTC ... ", current_cpu());
#endif  /*  DEBUG_UDELAY_CALIBRATION  */

	out_port_byte(MC146818_RTC_SELECT_REG, MC146818_RTC_REGA);

restart:
	/*
	 * 更新開始状態に合わせる
	 */
	val = in_port_byte(MC146818_RTC_READ_REG);
	while( !(val & ( 1 << MC146818_RTC_UPDATE_BIT ) ) )
		val = in_port_byte(MC146818_RTC_READ_REG);
	/*
	 * 更新完了を待つ
	 */
	val = in_port_byte(MC146818_RTC_READ_REG);
	while(val & ( 1 << MC146818_RTC_UPDATE_BIT ) )
		val = in_port_byte(MC146818_RTC_READ_REG);

	/*
	 * 更新開始を待つ
	 */
	val = in_port_byte(MC146818_RTC_READ_REG);
	while( !(val & ( 1 << MC146818_RTC_UPDATE_BIT ) ) )
		val = in_port_byte(MC146818_RTC_READ_REG);
	t1 = rdtsc();

	/*
	 * 更新ビットが落ちるのを待つ
	 */
	val = in_port_byte(MC146818_RTC_READ_REG);
	while(val & ( 1 << MC146818_RTC_UPDATE_BIT ) )
		val = in_port_byte(MC146818_RTC_READ_REG);


	/*
	 * 更新ビットが立つのを待つ
	 */
	do{

		val = in_port_byte(MC146818_RTC_READ_REG);
	}while( !( val & (1 << MC146818_RTC_UPDATE_BIT) ) );

	t2 = rdtsc();			
	if (t2 <= t1)
		goto restart;

	tsc_per_us = (t2 - t1) / 1000 / 1000;

#if defined(DEBUG_UDELAY_CALIBRATION)
	kprintf(KERN_DBG, "%u tsc/us\n", tsc_per_us);
#endif  /*  DEBUG_UDELAY_CALIBRATION  */

	_x86_64_set_tsc_per_us(tsc_per_us);
}

/** CPUを保持したままマイクロ秒待ちを行う
    @param[in] us ビジーループする時間(単位:マイクロ秒)
 */
void
hal_udelay(delay_cnt us) {
	uint64_t wait_tsc;

	wait_tsc = rdtsc() + (  _x86_64_get_tsc_per_us() * us );
	while( rdtsc() < wait_tsc );

	return;
}

/** udelayのセットアップを行う
 */
void
hal_setup_udelay(void) {

	x86_64_calcluate_tsc_per_sec();
}
