/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  event handler routines                                            */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/string.h>

#include <ulib/yatos-ulib.h>
#include <ulib/thread-svc.h>
#include <ulib/ev-handler.h>

static ev_handler_array uhdlrs;          /*  ユーザイベントハンドラ  */
static ev_handler_array default_hdlrs;  /*  デフォルトハンドラ  */

/** コアダンプ動作(現時点では, 終了動作)
    @param[in] id    イベントID
    @param[in] info  イベント情報
    @param[in] evf   イベントフレーム
 */
static void
ev_core_dump_handler(event_no id, evinfo __attribute__ ((unused)) *info,
    void __attribute__ ((unused)) *evf) {
	
	yatos_thread_exit(id);
	return;
}

/** 自スレッド終了デフォルトハンドラ
    @param[in] id    イベントID
    @param[in] info  イベント情報
    @param[in] evf   イベントフレーム
 */
static void
ev_term_handler(event_no id, evinfo __attribute__ ((unused)) *info, 
    void __attribute__ ((unused)) *evf) {

	yatos_thread_exit(id);
	return;
}

/** イベント無視デフォルトハンドラ
    @param[in] id    イベントID
    @param[in] info  イベント情報
    @param[in] evf   イベントフレーム
 */
static void
ev_ignore_handler(event_no __attribute__ ((unused)) id, 
    struct _evinfo __attribute__ ((unused)) *info, 
    void __attribute__ ((unused)) *evf) {

	return;
}

/** デフォルト動作を設定する
 */
static void
setup_default_handlers(void) {

	default_hdlrs.handlers[EV_SIG_HUP] = ev_term_handler;
	default_hdlrs.handlers[EV_SIG_INT] = ev_term_handler;
	default_hdlrs.handlers[EV_SIG_QUIT] = ev_core_dump_handler;
	default_hdlrs.handlers[EV_SIG_ILL] = ev_core_dump_handler;
	default_hdlrs.handlers[EV_SIG_ABRT] = ev_core_dump_handler;
	default_hdlrs.handlers[EV_SIG_FPE] = ev_core_dump_handler;
	default_hdlrs.handlers[EV_SIG_KILL] = ev_term_handler;
	default_hdlrs.handlers[EV_SIG_BUS] = ev_core_dump_handler;
	default_hdlrs.handlers[EV_SIG_SEGV] = ev_core_dump_handler;
	default_hdlrs.handlers[EV_SIG_PIPE] = ev_term_handler;
	default_hdlrs.handlers[EV_SIG_ALRM] = ev_term_handler;
	default_hdlrs.handlers[EV_SIG_TERM] = ev_term_handler;
	default_hdlrs.handlers[EV_SIG_CHLD] = ev_ignore_handler;
	default_hdlrs.handlers[EV_SIG_USR1] = ev_term_handler;
	default_hdlrs.handlers[EV_SIG_USR2] = ev_term_handler;
}

/** 指定のIDのハンドラを起動する
    @param[in] id    イベントID
    @param[in] info  イベント情報
    @param[in] evf   イベントフレーム
 */
void
__yatos_ulib_invoke_handler(event_no id, evinfo *info, event_frame *evf) {
	ev_handler handler;

	if ( ( is_ev_non_catchable_event(id) ) || ( id >= EV_NR_EVENT ) )
		return;

	handler = uhdlrs.handlers[id];

	handler(id, info, (void *)evf);
}

/** ユーザハンドラを登録する
    @param[in] id      イベントID
    @param[in] handler ハンドラのアドレス
    @retval    0       正常に登録できた
    @retval   -1       イベントIDが範囲外を指定している/捕捉不可能なイベントを指定している
 */
int
yatos_register_user_event_handler(event_no id, ev_handler handler){

	if ( is_ev_non_catchable_event(id) ) {

		set_errno(-EPERM);
		return -1;
	}

	if ( id >= EV_NR_EVENT ) {

		set_errno(-EINVAL);
		return -1;
	}

	if ( handler == EV_HDLR_DFL ) 
		uhdlrs.handlers[id] = default_hdlrs.handlers[id];
	else if ( handler == EV_HDLR_IGN ) 
		uhdlrs.handlers[id] = ev_ignore_handler;
	else 
		uhdlrs.handlers[id] = handler;

	return 0;
}

/** ユーザランドのハンドラを初期化する
 */
void
__yatos_user_event_handler_init(void) {
	int i;

	memset(&uhdlrs, 0, sizeof(ev_handler_array) );

	/*
	 * デフォルト動作を記憶
	 */
	for(i = 0; EV_NR_EVENT > i; ++i) {

		default_hdlrs.handlers[i] = ev_ignore_handler;  /*  デフォルトを無視にする  */
	}

	setup_default_handlers();  /* UNIXの標準動作を設定  */

	/*
	 * デフォルト動作を設定
	 */
	for(i = 0; EV_NR_EVENT > i; ++i) {

		uhdlrs.handlers[i] = default_hdlrs.handlers[i];
	}
}
