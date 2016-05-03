/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Userlland event handler  routines                                 */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <ulib/yatos-ulib.h>
#include <ulib/event-svc.h>
#include <ulib/thread-svc.h>
#include <ulib/ev-handler.h>

/**  共通イベントハンドラ
 */
static void
common_event_handler(event_id  no, evinfo  *info, event_frame *evf) {

	__yatos_ulib_invoke_handler(no, info, evf);
	yatos_event_return(evf);
}

/** イベントハンドラを登録する
 */
void
_yatos_register_common_event_handler(void) {
	syscall_res_type res;
	
	syscall1( res, SYS_YATOS_EV_REG_EVHANDLER, 
	    (syscall_arg_type)common_event_handler);
	set_errno(res);
}

/** イベントハンドラからカーネルに戻る
 */
void
yatos_event_return(event_frame *evf) {
	syscall_res_type res;
	
	syscall1( res, SYS_YATOS_EV_RETURN, evf);
	/*  Never return here */
}
