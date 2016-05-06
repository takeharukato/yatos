/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  main routines                                                     */
/*                                                                    */
/**********************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/assert.h>
#include <kern/string.h>
#include <kern/kprintf.h>
#include <kern/thread.h>
#include <kern/sched.h>
#include <kern/idle.h>
#include <kern/irq.h>
#include <kern/proc.h>
#include <kern/timer.h>
#include <kern/page.h>
#include <kern/kname-service.h>

#include <kern/tst-progs.h>

/** テストプログラムの起動定義
 */
void
_setup_test_progs(void) {
	
	/*  テストプログラム呼出はこれより下に記載 */

	//thread_test();
	//timer_test();
	//lpc1_test();
	//lpc2_test();
	//kserv_test();
	//wait_test();
	//thread_round_robin_test();
}

void
kcom_start_kernel(void) {

	kmalloc_cache_init();
	thr_idpool_init();
	sched_init_subsys();
	idle_init_subsys();
	irq_init_subsys();
	tim_init_subsys();

	hal_init_pic();

	kprintf(KERN_INF, "OS kernel\n");

	ti_disable_dispatch();

	system_threads_init();    /*  システムスレッドの起動  */
	hal_load_system_procs();  /*  システムプロセスの起動  */

	hal_release_boot_time_resources();

	ti_enable_dispatch();



	idle_start();
}

