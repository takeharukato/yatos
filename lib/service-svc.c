/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  name service routines                                             */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <ulib/yatos-ulib.h>
#include <ulib/service-svc.h>
#include <ulib/thread-svc.h>
#include <ulib/lpc-svc.h>

/** 自スレッドをサービス提供者として登録する
    @param[in] name  サービス名
    @retval    0     正常終了
    @retval    負    登録失敗
 */
int
yatos_register_service(const char *name){
	int                  rc;
	msg_body            msg;
	kname_service_msg *nsrv;

	if ( name == NULL ) {

		set_errno(-EFAULT);
		return -1;
	}

	nsrv = &msg.kname_msg;
	
	memset( &msg, 0, sizeof(msg_body) );
	nsrv->req = KSERV_REG_SERVICE;
	nsrv->name = name;
	nsrv->len = strlen(nsrv->name);
	nsrv->id = yatos_thread_getid();
	rc = yatos_lpc_send_and_reply(ID_RESV_NAME_SERV, &msg);
	if ( rc < 0 ) {

		set_errno(nsrv->rc);
		rc = -1;
	}

	return rc;
}

/** サービスの登録を破棄する
    @param[in] name  サービス名
    @retval    0     正常終了
    @retval    負    登録失敗
 */
int
yatos_unregister_service(const char *name) {
	int                  rc;
	msg_body            msg;
	kname_service_msg *nsrv;

	if ( name == NULL ) {

		set_errno(-EFAULT);
		return -1;
	}

	nsrv = &msg.kname_msg;
	
	memset( &msg, 0, sizeof(msg_body) );
	nsrv->req = KSERV_UNREG_SERVICE;
	nsrv->name = name;
	nsrv->len = strlen(nsrv->name);

	rc = yatos_lpc_send_and_reply(ID_RESV_NAME_SERV, &msg);
	if ( rc < 0 ) {

		set_errno(nsrv->rc);
		rc = -1;
	}

	return rc;
}

/** サービス名からサービス提供エンドポイントを探査する
    @param[in] name  サービス名
    @param[out] ep   エンドポイント格納先アドレス
    @retval    0     正常終了
    @retval    負    登録失敗
 */
int 
yatos_lookup_service(const char *name, endpoint *ep){
	int                  rc;
	msg_body            msg;
	kname_service_msg *nsrv;

	if ( ( name == NULL ) || ( ep == NULL ) ) {

		set_errno(-EFAULT);
		return -1;
	}

	nsrv = &msg.kname_msg;
	
	memset( &msg, 0, sizeof(msg_body) );
	nsrv->req = KSERV_LOOKUP;
	nsrv->name = name;
	nsrv->len = strlen(nsrv->name);

	rc = yatos_lpc_send_and_reply(ID_RESV_NAME_SERV, &msg);
	if ( rc < 0 ) {

		set_errno(rc);
		return -1;
	}
	if ( nsrv->rc != 0 ) {

		set_errno(nsrv->rc);
		return -1;
	}

	*ep = nsrv->id;

	return 0;
}
