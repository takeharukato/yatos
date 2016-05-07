/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  process system call routines                                      */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <ulib/yatos-ulib.h>
#include <ulib/proc-svc.h>
#include <ulib/thread-svc.h>
#include <ulib/lpc-svc.h>

/** ユーザスレッドランチャ
    @param[in] uthr_start ユーザスレッド開始アドレス
    @param[in] arg        ユーザスレッド引数
    @param[in] extra_arg  拡張引数(未使用)
 */
void
__yatos_user_thread_launcher(int (*uthr_start)(void *), void *arg,
    void  __attribute__ ((unused)) *extra_arg) {
	int rc;

	rc = uthr_start(arg);
	yatos_thread_exit(rc);
}

/** 指定したスレッドにイベントを送出する
    @param[in] dest 送信先スレッドのID
    @param[in] id   イベントID
    @param[in] data イベントの付帯情報
 */
int
yatos_proc_send_event(tid dest, event_id  id, event_data data) {
	int                      rc;
	msg_body                msg;
	proc_service          *pmsg;
	proc_sys_send_event  *sndev;

	pmsg= &msg.proc_msg;
	sndev = &pmsg->proc_service_calls.sndev;

	memset( &msg, 0, sizeof(msg_body) );

	pmsg->req = PROC_SERV_REQ_SNDEV;
	sndev->dest = dest;
	sndev->type = PROC_SERV_SNDEV_THR;
	sndev->id = id;
	sndev->data = data;	

	rc = yatos_lpc_send_and_reply( ID_RESV_PROC, &msg );
	if ( rc != 0 ) {

		set_errno(rc);
		return -1;
	}

	if ( pmsg->rc != 0 ) {

		set_errno(pmsg->rc);
		return -1;
	}

	return 0;
}

/** 指定したスレッドが所属するプロセスにイベントを送出する
    @param[in] dest 送信先スレッドのID
    @param[in] id   イベントID
    @param[in] data イベントの付帯情報
 */
int
yatos_proc_send_proc_event(tid dest, event_id  id, event_data data) {
	int                      rc;
	msg_body                msg;
	proc_service          *pmsg;
	proc_sys_send_event  *sndev;

	pmsg= &msg.proc_msg;
	sndev = &pmsg->proc_service_calls.sndev;

	memset( &msg, 0, sizeof(msg_body) );

	pmsg->req = PROC_SERV_REQ_SNDEV;
	sndev->dest = dest;
	sndev->type = PROC_SERV_SNDEV_PROC;
	sndev->id = id;
	sndev->data = data;	

	rc = yatos_lpc_send_and_reply( ID_RESV_PROC, &msg );

	if ( rc != 0 ) {

		set_errno(rc);
		return -1;
	}

	if ( pmsg->rc != 0 ) {

		set_errno(pmsg->rc);
		return -1;
	}

	return 0;
}

/** 指定したスレッドが所属する全スレッドにイベントを送出する
    @param[in] dest 送信先スレッドのID
    @param[in] id   イベントID
    @param[in] data イベントの付帯情報
 */
int
yatos_proc_bcast_proc_event(tid dest, event_id  id, event_data data) {
	int                      rc;
	msg_body                msg;
	proc_service          *pmsg;
	proc_sys_send_event  *sndev;

	pmsg= &msg.proc_msg;
	sndev = &pmsg->proc_service_calls.sndev;

	memset( &msg, 0, sizeof(msg_body) );

	pmsg->req = PROC_SERV_REQ_SNDEV;
	sndev->dest = dest;
	sndev->type = PROC_SERV_SNDEV_ALLTHR;
	sndev->id = id;
	sndev->data = data;	

	rc = yatos_lpc_send_and_reply( ID_RESV_PROC, &msg );
	if ( rc != 0 ) {

		set_errno(rc);
		return -1;
	}

	if ( pmsg->rc != 0 ) {

		set_errno(pmsg->rc);
		return -1;
	}

	return 0;
}

/** ユーザスレッドを生成する
    @param[in] prio   生成するスレッドの優先度
    @param[in] start  開始アドレス
    @param[in] arg    引数
    @param[in] sp     スタックポインタ
    @param[in] newidp スレッドidの格納先アドレス
 */
int
yatos_proc_create_thread(thr_prio prio, int (*start)(void *), void *arg, void *sp, tid *newidp) {
	int                          rc;
	msg_body                    msg;
	proc_service              *pmsg;
	proc_sys_create_thread  *crethr;

	if ( newidp == NULL )
		return -EINVAL;

	pmsg= &msg.proc_msg;
	crethr = &pmsg->proc_service_calls.crethr;

	memset( &msg, 0, sizeof(msg_body) );

	pmsg->req = PROC_SERV_REQ_CRETHR;
	crethr->prio = prio;
	crethr->start = (void *)__yatos_user_thread_launcher;
	crethr->arg1 = start;
	crethr->arg2 = arg;
	crethr->arg3 = NULL;
	crethr->sp = sp;

	rc = yatos_lpc_send_and_reply( ID_RESV_PROC, &msg );
	if ( rc != 0 ) {

		set_errno(rc);
		return -1;
	}

	if ( pmsg->rc != 0 ) {

		set_errno(pmsg->rc);
		return -1;
	}

	*newidp = crethr->id;

	return 0;
}

/** 指定したスレッドの消費資源量を獲得する
    @param[in] id   対象スレッドのID
    @param[in] resp リソース情報格納先アドレス
 */
int
yatos_proc_get_thread_resource(tid id, thread_resource *resp) {
	int                        rc;
	msg_body                  msg;
	proc_service            *pmsg;
	proc_sys_getrusage   *getrusg;

	pmsg= &msg.proc_msg;
	getrusg = &pmsg->proc_service_calls.getrusg;

	memset( &msg, 0, sizeof(msg_body) );

	pmsg->req = PROC_SERV_REQ_GETRUSAGE;
	getrusg->id = id;

	rc = yatos_lpc_send_and_reply( ID_RESV_PROC, &msg );
	if ( rc != 0 ) {

		set_errno(rc);
		return -1;
	}

	if ( pmsg->rc != 0 ) {

		set_errno(pmsg->rc);
		return -1;
	}

	memcpy( resp, &getrusg->res, sizeof(thread_resource) );

	return 0;
}
