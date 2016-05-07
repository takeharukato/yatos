/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  System thread initialization                                      */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/thread.h>
#include <kern/messages.h>
#include <kern/tst-progs.h>
#include <kern/kname-service.h>

#include <thr/thr-internal.h>


/** システムスレッドを初期化する
 */
void
system_threads_init(void) {

	ti_disable_dispatch();

	_thr_init_reaper();          /*  スレッド回収処理             */
	kernel_name_service_init();  /* カーネル内ネームサービス      */
	dbg_console_service_init();  /* デバッグ用コンソールサービス  */
	thr_service_init();          /* スレッドサービスコール        */
	proc_service_init();         /* プロセスサービスコール        */
	vm_service_init();           /* VMサービスコール              */

	_setup_test_progs();
	ti_enable_dispatch();
}
