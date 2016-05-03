/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Kernel service test routines                                      */
/*                                                                    */
/**********************************************************************/

#include <stddef.h>
#include <stdint.h>

#include <kern/config.h>
#include <kern/assert.h>
#include <kern/string.h>
#include <kern/kprintf.h>
#include <kern/thread.h>
#include <kern/sched.h>
#include <kern/proc.h>
#include <kern/lpc.h>
#include <kern/kname-service.h>
#include <kern/vm.h>
#include <kern/kresv-ids.h>

#include <kern/tst-progs.h>

static thread *thrA, *thrB;

#define SERV_NAME "FakeConsole"

int
kthreadA(void __attribute__ ((unused)) *arg) {
	int                  rc;
	int               count;
	msg_body            msg;
	kname_service_msg *nsrv;
	pri_string_msg    *pmsg;
	endpoint            con;

	kprintf(KERN_INF, "kthreadA: tid=%d thread=%p\n", 
	    current->tid, current);

	nsrv= &msg.kname_msg;
	count = 0;
	do {
		++count;
		memset( &msg, 0, sizeof(msg_body) );
		nsrv->req = KSERV_LOOKUP;
		nsrv->name = SERV_NAME;
		nsrv->len = strlen(nsrv->name);
		nsrv->id = 0;

		rc = lpc_send_and_reply(ID_RESV_NAME_SERV, &msg);
		kassert( rc == 0);

		kprintf(KERN_INF, 
		    "kthreadA: lookup tid=%d rc=%d, lookup-result=%d, serv-id=%d\n", 
		    current->tid, rc, nsrv->rc, nsrv->id);
	}while( ( rc == 0 ) && ( nsrv->rc == -ENOENT ) && (count < 10 ) );

	if ( ( count < 10 ) && ( nsrv->rc == 0 ) ){

		kprintf(KERN_INF, 
		    "kthreadA: lookup tid=%d rc=%d, lookup-result=%d, "
		    "serv-id=%d OK\n",  current->tid, rc, nsrv->rc, nsrv->id);
	}

	con = nsrv->id;

	pmsg= &msg.sys_pri_dbg_msg;
	memset( &msg, 0, sizeof(msg_body) );
	pmsg->req = 0;
	pmsg->msg = "Hello World\n";
	pmsg->len = strlen(pmsg->msg) + 1;

	kprintf(KERN_INF, 
	    "kthreadA: try to send tid=%d dest=%d msg=%p %s\n", 
	    current->tid, con, pmsg->msg, pmsg->msg);
	rc = lpc_send_and_reply(con, &msg);
	kprintf(KERN_INF, 
	    "kthreadA: send return tid=%d rc=%d, print-rc=%d\n", 
		    current->tid, rc, pmsg->rc);

	/*
	 * 本物のコンソールサービスにメッセージを出す
	 */
	pmsg= &msg.sys_pri_dbg_msg;
	memset( &msg, 0, sizeof(msg_body) );
	pmsg->req = 0;
	pmsg->msg = "Hello World\n";
	pmsg->len = strlen(pmsg->msg) + 1;

	kprintf(KERN_INF, 
	    "kthreadA: try to send real debug console tid=%d dest=%d msg=%p %s len=%d\n", 
	    current->tid, ID_RESV_DBG_CONSOLE, pmsg->msg, pmsg->msg, pmsg->len);
	rc = lpc_send_and_reply(ID_RESV_DBG_CONSOLE, &msg);
	kprintf(KERN_INF, 
	    "kthreadA: send return tid=%d rc=%d, print-rc=%d\n", 
		    current->tid, rc, pmsg->rc);

	kprintf(KERN_INF, "kthreadA: exit tid=%d thread=%p\n", 
	    current->tid, current);

	thr_exit(0);
	return 0;
}

int
kthreadB(void __attribute__ ((unused)) *arg) {
	thread *req_thr;
	msg_body msg;
	kname_service_msg *nsrv;
	pri_string_msg    *pmsg;
	endpoint            src;
	int rc;

	kprintf(KERN_INF, SERV_NAME ":tid=%d thread=%p\n", 
	    current->tid, current);

	/*
	 * 登録テスト
	 */
	nsrv= &msg.kname_msg;
	memset( &msg, 0, sizeof(msg_body) );

	nsrv->req = KSERV_REG_SERVICE;
	nsrv->name = SERV_NAME;
	nsrv->len = strlen(nsrv->name);
	nsrv->id = current->tid;
	
	rc = lpc_send_and_reply(ID_RESV_NAME_SERV, &msg);
	kassert( rc == 0 );

	if ( nsrv->rc == 0 )
		kprintf(KERN_INF, SERV_NAME ":tid=%d rc=%d, register-rc=%d OK\n", 
		    current->tid, rc, nsrv->rc);
	else {
		kprintf(KERN_INF, SERV_NAME ":tid=%d rc=%d, register-rc=%d NG\n", 
		    current->tid, rc, nsrv->rc);
		while(1);
	}

	/*
	 * 多重登録チェックのテスト
	 */
	nsrv= &msg.kname_msg;
	memset( &msg, 0, sizeof(msg_body) );

	nsrv->req = KSERV_REG_SERVICE;
	nsrv->name = SERV_NAME;
	nsrv->len = strlen(nsrv->name);
	nsrv->id = current->tid;
	
	rc = lpc_send_and_reply(ID_RESV_NAME_SERV, &msg);
	kassert( rc == 0 );

	if ( nsrv->rc == 0 ) {

		kprintf(KERN_INF, SERV_NAME ":tid=%d rc=%d, register-rc=%d NG\n", 
		    current->tid, rc, nsrv->rc);
		while(1);
	} else {

		kprintf(KERN_INF, SERV_NAME ":tid=%d rc=%d, register-rc=%d OK\n", 
		    current->tid, rc, nsrv->rc);
	}

	while(1) {

		/*
		 * サービスのテスト
		 */
		memset( &msg, 0, sizeof(msg_body) );
		pmsg  = &msg.sys_pri_dbg_msg;

		kprintf(KERN_DBG, SERV_NAME ":recv\n");
		
		rc = lpc_recv(LPC_RECV_ANY, LPC_INFINITE, &msg, &src);
		kassert( rc == 0 );

		kprintf(KERN_DBG, SERV_NAME ":recv:rc=%d src=%d msg=[%p] len=%d\n",
		    rc, src, pmsg->msg, pmsg->len);

		req_thr = thr_find_thread_by_tid(src);
		if ( req_thr == NULL ) {

			kprintf(KERN_DBG, SERV_NAME ":src=%d not found.\n",
			    rc, src);
			continue;
		}
		if ( !vm_user_area_can_access(&req_thr->p->vm, pmsg->msg, pmsg->len,
			VMA_PROT_R) ) {

			kprintf(KERN_DBG, 
			    SERV_NAME ":Can not access %p length: %d by tid=%d\n",
			    pmsg->msg, pmsg->len, req_thr->tid);

			pmsg->rc = -EPERM;
			rc = lpc_send(src, LPC_INFINITE, &msg);
			kassert( rc == 0 );
			continue;
		} else {
			
			kprintf(KERN_INF, "recv: %s", pmsg->msg);
			pmsg->rc = pmsg->len;
			rc = lpc_send(src, LPC_INFINITE, &msg);
			kassert( rc == 0 );
			break;
		}
	}

	/*
	 * サービス登録解除
	 */
	nsrv= &msg.kname_msg;
	memset( &msg, 0, sizeof(msg_body) );

	nsrv->req = KSERV_UNREG_SERVICE;
	nsrv->name = SERV_NAME;
	nsrv->len = strlen(nsrv->name);
	
	rc = lpc_send_and_reply(ID_RESV_NAME_SERV, &msg);
	kassert( rc == 0 );
	if ( nsrv->rc == 0 ) {

		kprintf(KERN_INF, SERV_NAME ":tid=%d rc=%d, unregister-rc=%d OK\n", 
		    current->tid, rc, nsrv->rc);
	} else {

		kprintf(KERN_INF, SERV_NAME ":tid=%d rc=%d, unregister-rc=%d NG\n", 
		    current->tid, rc, nsrv->rc);
		while(1);
	}
	/*
	 * サービス登録解除の確認テスト
	 */
	memset( &msg, 0, sizeof(msg_body) );
	nsrv->req = KSERV_LOOKUP;
	nsrv->name = SERV_NAME;
	nsrv->len = strlen(nsrv->name);
	nsrv->id = 0;
	
	rc = lpc_send_and_reply(ID_RESV_NAME_SERV, &msg);
	kassert( rc == 0);
	if ( nsrv->rc == -ENOENT ) {

		kprintf(KERN_INF, 
		    SERV_NAME ": lookup with no entry"
		    " tid=%d rc=%d, lookup-result=%d, serv-id=%d OK\n", 
		    current->tid, rc, nsrv->rc, nsrv->id);
	} else {

		kprintf(KERN_INF, 
		    SERV_NAME ": lookup with no entry "
		    "tid=%d rc=%d, lookup-result=%d, serv-id=%d NG\n", 
		    current->tid, rc, nsrv->rc, nsrv->id);
		while(1);
	}
	/*
	 * サービス登録解除のエラー系テスト
	 */
	nsrv= &msg.kname_msg;
	memset( &msg, 0, sizeof(msg_body) );

	nsrv->req = KSERV_UNREG_SERVICE;
	nsrv->name = SERV_NAME;
	nsrv->len = strlen(nsrv->name);
	
	rc = lpc_send_and_reply(ID_RESV_NAME_SERV, &msg);
	kassert( rc == 0 );
	if ( nsrv->rc == 0 ) {

		kprintf(KERN_INF, SERV_NAME ": unregister with no entry "
		    ":tid=%d rc=%d, unregister-rc=%d NG\n", 
		    current->tid, rc, nsrv->rc);
		while(1);
	} else {

		kprintf(KERN_INF, SERV_NAME ": unregister with no entry "
		    ":tid=%d rc=%d, unregister-rc=%d OK\n", 
		    current->tid, rc, nsrv->rc);
	}

	kprintf(KERN_INF, SERV_NAME ": exit tid=%d thread=%p\n", 
	    current->tid, current);

	return 0;
}


void
kserv_test(void) {
	int              rc;

	rc = thr_new_thread(&thrA);
	kassert( rc == 0 );

	rc = thr_new_thread(&thrB);
	kassert( rc == 0 );

	rc = thr_create_kthread(thrA, 0, THR_FLAG_NONE, THR_INVALID_TID, 
	    kthreadA, (void *)"ThreadA");
	kassert( rc == 0 );

	rc = thr_create_kthread(thrB, 0, THR_FLAG_NONE, THR_INVALID_TID, 
	    kthreadB, (void *)SERV_NAME);
	kassert( rc == 0 );

	rc = thr_start(thrA, current->tid);
	kassert( rc == 0 );

	rc = thr_start(thrB, current->tid);
	kassert( rc == 0 );
}

