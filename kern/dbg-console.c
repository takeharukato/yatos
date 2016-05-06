/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Debug console service relevant routines                           */
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
#include <kern/thread.h>
#include <kern/sched.h>
#include <kern/proc.h>
#include <kern/lpc.h>
#include <kern/kname-service.h>
#include <kern/vm.h>
#include <kern/kresv-ids.h>

static thread *dbg_console_thr;

//#define DEBUG_DBG_CON

#define MSG_PREFIX "debug console:"

static int
dbg_console_thread(void __attribute__ ((unused)) *arg) {
	intrflags         flags;
	thread         *req_thr;
	msg_body            msg;
	pri_string_msg    *pmsg;
	endpoint            src;
	int                  rc;

#if defined(DEBUG_DBG_CON)
	kprintf(KERN_INF, MSG_PREFIX "tid=%d thread=%p\n", 
	    current->tid, current);
#endif  /*  DEBUG_DBG_CON  */

	/* デバッグ用コンソールサービスをカーネル内のネームサービスに登録
	 */
	rc = kns_register_kernel_service(ID_RESV_NAME_DBG_CONSOLE);
	kassert( rc == 0 );
#if defined(DEBUG_DBG_CON)
	kprintf(KERN_INF, MSG_PREFIX "Register \"%s\" service by tid=%d rc=%d \n", 
	    ID_RESV_NAME_DBG_CONSOLE, current->tid, rc);
#endif  /*  DEBUG_DBG_CON  */

	while(1) {

		/* メッセージを受信
		 */
		memset( &msg, 0, sizeof(msg_body) );
		pmsg  = &msg.sys_pri_dbg_msg;

		rc = lpc_recv(LPC_RECV_ANY, LPC_INFINITE, &msg, &src);
		kassert( rc == 0 );

#if defined(DEBUG_DBG_CON)
		kprintf(KERN_DBG, MSG_PREFIX "recv:rc=%d src=%d msg=[%p] len=%d\n",
		    rc, src, pmsg->msg, pmsg->len);
#endif  /*  DEBUG_DBG_CON  */

		/* 送信者スレッドを取得
		 */
		acquire_all_thread_lock( &flags );
		
		req_thr = thr_find_thread_by_tid_nolock(src);
		if ( req_thr == NULL ) {

#if defined(DEBUG_DBG_CON)
			kprintf(KERN_DBG, MSG_PREFIX "src=%d not found.\n",
			    rc, src);
#endif  /*  DEBUG_DBG_CON  */
			release_all_thread_lock(&flags);
			continue;
		}

		/* 送信データ領域の妥当性を確認
		 */
		if ( !vm_user_area_can_access(&req_thr->p->vm, pmsg->msg, pmsg->len,
			VMA_PROT_R) ) {

#if defined(DEBUG_DBG_CON)
			kprintf(KERN_DBG, 
			    MSG_PREFIX "Can not access %p length: %d by tid=%d\n",
			    pmsg->msg, pmsg->len, req_thr->tid);
#endif  /*  DEBUG_DBG_CON  */

			release_all_thread_lock(&flags);

			/* 領域外アクセスで復帰
			 */
			pmsg->rc = -EFAULT;
			rc = lpc_send(src, LPC_INFINITE, &msg);
			kassert( rc == 0 );
			continue;
		}

		/* ユーザランドサービスの場合は, 文字列情報をコピーして処理する必要が
		 * あるがカーネル内サービスの場合は, カーネル空間をプロセス間で共有
		 * していることから, コピー用のメモリとコピー時間の削減のため
		 * 一時的に要求元のアドレス空間に切り替えてアクセスする。
		 */
		hal_switch_address_space( current->p,  req_thr->p);
		kprintf(KERN_INF, "%s", pmsg->msg);
		hal_switch_address_space( req_thr->p, current->p);

		release_all_thread_lock(&flags);

		/* 送信メッセージ長を返却
		 */
		pmsg->rc = pmsg->len;
		rc = lpc_send(src, LPC_INFINITE, &msg);
		kassert( rc == 0 );
	}

	return 0;
}

/** デバッグ用コンソールサービスを初期化する
 */
void
dbg_console_service_init(void) {
	int              rc;

	rc = thr_new_thread(&dbg_console_thr);
	kassert( rc == 0 );

	rc = thr_create_kthread(dbg_console_thr, KSERV_DBG_CONS_PRIO, THR_FLAG_NONE,
	    ID_RESV_DBG_CONSOLE, dbg_console_thread, (void *)ID_RESV_NAME_DBG_CONSOLE);
	kassert( rc == 0 );

	rc = thr_start(dbg_console_thr, current->tid);
	kassert( rc == 0 );

}
