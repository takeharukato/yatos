/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Local Process Communication relevant systemcall routines          */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <ulib/yatos-ulib.h>
#include <ulib/lpc-svc.h>

/** LPCメッセージを送信する
    @param[in] dest  送信先エンドポイント
    @param[in] tmout タイムアウト時間(単位:ms)
    @param[in] m     送信電文
    @retval    0     正常に送信した
 */
int
yatos_lpc_send(endpoint dest, lpc_tmout tmout, void *m){
	syscall_res_type res;

	syscall3( res, SYS_YATOS_LPC_SEND, 
	    (syscall_arg_type)dest, 
	    (syscall_arg_type)tmout, 
	    (syscall_arg_type)m	);

	set_errno(res);

	if ( res < 0 )
		return -1;

	return 0;
}

/** LPCメッセージを受信する
    @param[in] src   送信元エンドポイント
    @param[in] tmout タイムアウト時間(単位:ms)
    @param[in] m     送信電文
    @retval    0     正常に送信した
 */
int 
yatos_lpc_recv(endpoint src, lpc_tmout tmout, void *m, endpoint *sender){
	syscall_res_type res;

	syscall4( res, SYS_YATOS_LPC_RECV, 
	    (syscall_arg_type)src, 
	    (syscall_arg_type)tmout, 
	    (syscall_arg_type)m,
	    (syscall_arg_type)sender);

	set_errno(res);

	if ( res < 0 )
		return -1;

	return 0;
}

/** LPCメッセージを送信後リプライを待ち合わせる
    @param[in] dest  送信先エンドポイント
    @param[in] tmout タイムアウト時間(単位:ms)
    @param[in] m     送信電文
    @retval    0     正常に送信した
 */
int
yatos_lpc_send_and_reply(endpoint dest, void *m) {
	syscall_res_type res;

	syscall2( res, SYS_YATOS_LPC_SEND_AND_REPLY, 
	    (syscall_arg_type)dest, 
	    (syscall_arg_type)m);

	set_errno(res);

	if ( res < 0 )
		return -1;

	return 0;
}
