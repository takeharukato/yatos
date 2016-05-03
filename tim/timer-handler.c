/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  timer interrupt handler relevant routines                         */
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
#include <kern/queue.h>
#include <kern/list.h>
#include <kern/thread.h>
#include <kern/irq.h>
#include <kern/timer.h>

#include <tim/tim-internal.h>

uptime_ticks uptime;  /*<  起動後の総ティック発生回数  */

/** タイマハンドラ
    @param[in] no   割込み番号
    @param[in] data プライベートデータ
    @param[in] ctx  割込みコンテキスト
    @retval IRQHDL_RES_HANDLED  タイマ割込みを処理した
 */
static ihandler_res 
timer_handler(intr_no __attribute__ ((unused)) no, private_inf __attribute__ ((unused)) data, void *ctx) {
	intrflags flags;
	ticks cur_tick;

	spinlock_lock_disable_intr( &uptime.lock, &flags);

	++uptime.tick_cnt;
	cur_tick = uptime.tick_cnt;

	spinlock_unlock_restore_intr( &uptime.lock, &flags);

	if ( current->p != hal_refer_kernel_proc() ) {
		
		/*
		 * ユーザスレッドの場合は, CPU消費資源量を更新
		 */
		if ( hal_is_intr_from_user(ctx) )
			++current->resource.user_time;
		else
			++current->resource.sys_time;
	}

	_tim_invoke_callout(cur_tick);

	return IRQHDL_RES_HANDLED;
}

/**  現在のアップタイムをロックフリープロトコルで獲得する
 */
ticks
_tim_refer_uptime_lockfree(void) {
	ticks cur_time1;
	ticks cur_time2;
	
	do{
		cur_time1 = uptime.tick_cnt;
		cur_time2 = uptime.tick_cnt;
	}while( cur_time1 != cur_time2 );

	return cur_time1;
}

/** アップタイムクロックの初期化
 */
void
_tim_setup_uptime_clock(void) {

	spinlock_init( &uptime.lock );
	uptime.tick_cnt = 0;
}

/** タイマ割込みハンドラの割込み番号を登録する
    @param[in] no   タイマの割込み番号
    @param[in] data タイマハンドラのプライベート情報
    @retval  -ENOMEM   メモリが不足している
    @retval  -EBUSY    既に同一のハンドラが同一のIRQに登録されている
    @retval  -EPERM    割込みの占有が行えなかったまたは直接関数呼び出し型のハンドラと
                       非関数呼び出し型のハンドラとで割込み線を共有させようとした
 */
int
kcom_tim_register_timer_irq(intr_no no,  private_inf data) {
	int rc;

	kassert( no < NR_IRQS );

	rc = irq_register_irq_handler(no, timer_handler, 
	    IRQHDL_FLAG_EXCLUSIVE, data);

	if ( rc == 0 )
		irq_unset_mask(no);

	return rc;
}

/** タイマ割込みハンドラの割込み番号の登録を抹消する
    @param[in] no   タイマの割込み番号
    @retval    0       正常に登録を抹消した
    @retval  -ENOENT   引数で指定したハンドラが登録されていない
 */
int
kcom_tim_unregister_timer_irq(intr_no no) {
	int rc;

	rc = irq_unregister_irq_handler(no, timer_handler);

	return rc;
}
