/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  thread service routines                                           */
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
#include <kern/proc.h>
#include <kern/thread.h>
#include <kern/vm.h>
#include <kern/lpc.h>
#include <kern/kname-service.h>

static thread *thr_service_thr;

//#define DEBUG_THR_SERVICE

#define MSG_PREFIX "THR:"

/** スレッドのイベントマスクを取得するシステムコール(実装部)
    @param[in] mask   イベントマスク返却先
    @param[in] src    送信元エンドポイント
    @retval    0      正常に返却した
    @retval   -ENOENT 要求元スレッドが消失
 */
static int
handle_get_evmask(thr_sys_mask_op *mask, endpoint src) {
	int            rc;
	intrflags   flags;
	thread       *thr;

	kassert( mask != NULL );

	acquire_all_thread_lock( &flags );

	thr = thr_find_thread_by_tid_nolock(src);
	if ( thr == NULL ) {

		rc = -ENOENT;
		release_all_thread_lock(&flags);
		goto error_out;
	}

	memcpy( &mask->mask, &thr->evque.masks, sizeof(event_mask) );
	release_all_thread_lock(&flags);

	return 0;

error_out:
	return rc;
}

/** スレッドのイベントマスクを設定するシステムコール(実装部)
    @param[in] mask   イベントマスク取得元
    @param[in] src    送信元エンドポイント
    @retval    0      正常に設定した
    @retval   -ENOENT 要求元スレッドが消失
 */
static int
handle_set_evmask(thr_sys_mask_op *mask, endpoint src) {
	int            rc;
	intrflags   flags;
	thread       *thr;

	kassert( mask != NULL );

	acquire_all_thread_lock( &flags );

	thr = thr_find_thread_by_tid_nolock(src);
	if ( thr == NULL ) {

		rc = -ENOENT;
		release_all_thread_lock(&flags);
		goto error_out;
	}
	ev_mask_unset( &mask->mask, EV_SIG_KILL);  /*  KILLだけは開けておく  */

	ev_update_mask(thr, &mask->mask);   /*  マスクを設定する  */

	release_all_thread_lock(&flags);

	return 0;

error_out:
	return rc;
}


/** スレッドサービス処理部
    @param[in] arg スレッド引数(未使用)
 */
static int
handle_thr_service(void __attribute__ ((unused)) *arg) {
	msg_body           msg;
	thr_service      *smsg;
	thr_sys_mask_op *mskop;
	endpoint           src;
	int                 rc;

	rc = kns_register_kernel_service(ID_RESV_NAME_THR);
	kassert( rc == 0 );
	
	while(1) {

		memset( &msg, 0, sizeof(msg_body) );
		smsg  = &msg.thr_msg;

		rc = lpc_recv(LPC_RECV_ANY, LPC_INFINITE, &msg, &src);
		kassert( rc == 0 );

#if defined(DEBUG_THR_SERVICE)
		kprintf(KERN_INF, MSG_PREFIX "tid=%d thread=%p (src, req)=(%d, %d)\n", 
		    current->tid, current, src, smsg->req);
#endif  /*  DEBUG_THR_SERVICE  */

		switch( smsg->req ) {
		case THR_SERV_REQ_GET_EVMSK:

			mskop = &smsg->thr_service_calls.maskop;
			smsg->rc = handle_get_evmask(mskop, src);

			rc = lpc_send(src, LPC_INFINITE, &msg);
			kassert( rc == 0 );
			break;
		case THR_SERV_REQ_SET_EVMSK:

			mskop = &smsg->thr_service_calls.maskop;
			smsg->rc = handle_set_evmask(mskop, src);

			rc = lpc_send(src, LPC_INFINITE, &msg);
			kassert( rc == 0 );
			break;
		default:
			smsg->rc = -ENOSYS;
			rc = lpc_send(src, LPC_INFINITE, &msg);
			kassert( rc == 0 );
			break;
		}
	}

	return 0;
}

/**
 */
void
thr_service_init(void) {
	int              rc;

	rc = thr_new_thread(&thr_service_thr);
	kassert( rc == 0 );

	rc = thr_create_kthread(thr_service_thr, KSERV_KSERVICE_PRIO, THR_FLAG_NONE,
	    ID_RESV_THR, handle_thr_service, (void *)ID_RESV_NAME_THR);
	kassert( rc == 0 );

	rc = thr_start(thr_service_thr, current->tid);
	kassert( rc == 0 );

}
