/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  service routines                                          */
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

static thread *YYYY_service_thr;

//#define DEBUG_XXXX_SERVICE

#define MSG_PREFIX "XXXX:"

/** 
    @param[in] arg スレッド引数(未使用)
 */
static int
handle_YYYY_service(void __attribute__ ((unused)) *arg) {
	msg_body          msg;
	YYYY_service    *smsg;
	endpoint          src;
	int                rc;

	rc = kns_register_kernel_service(ID_RESV_NAME_XXXX);
	kassert( rc == 0 );
	
	while(1) {

		memset( &msg, 0, sizeof(msg_body) );
		smsg  = &msg.YYYY_msg;

		rc = lpc_recv(LPC_RECV_ANY, LPC_INFINITE, &msg, &src);
		kassert( rc == 0 );

#if defined(DEBUG_XXXX_SERVICE)
		kprintf(KERN_INF, MSG_PREFIX "tid=%d thread=%p (src, req)=(%d, %d)\n", 
		    current->tid, current, src, smsg->req);
#endif  /*  DEBUG_XXXX_SERVICE  */

		switch( smsg->req ) {
		case :
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
YYYY_service_init(void) {
	int              rc;

	rc = thr_new_thread(&YYYY_service_thr);
	kassert( rc == 0 );

	rc = thr_create_kthread(YYYY_service_thr, KSERV_KSERVICE_PRIO, THR_FLAG_NONE,
	    ID_RESV_XXXX, handle_YYYY_service, (void *)ID_RESV_NAME_XXXX);
	kassert( rc == 0 );

	rc = thr_start(YYYY_service_thr, current->tid);
	kassert( rc == 0 );

}
