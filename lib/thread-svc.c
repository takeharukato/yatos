/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Thread relevant systemcall routines                               */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <ulib/yatos-ulib.h>
#include <ulib/thread-svc.h>

/** CPUを開放する
 */
void
yatos_thread_yield(void) {
	syscall_res_type res;

	syscall0( res, SYS_YATOS_THR_YIELD);
}

/** 自スレッドを終了する
    @param[in] rc 終了コード
 */
void
yatos_thread_exit(int rc) {
	syscall_res_type res;

	syscall1( res, SYS_YATOS_THR_EXIT, (syscall_arg_type)rc);
}

/** 自スレッドのIDを取得する
 */
tid
yatos_thread_getid(void) {
	syscall_res_type res;

	syscall0( res, SYS_YATOS_THR_GETID );

	return (tid)res;
}

/** 子スレッドの終了を待ち合わせる
    @param[in]  tid    待ち合わせ対象スレッドのスレッドID
    @param[in]  wflags 待ち合わせ対象スレッドの指定
    @param[out] exit_tidp 終了したスレッドのID返却先
    @param[out] rcp    子スレッドの終了コード格納アドレス 
    @retval    0      待ち合わせ完了
    @retval   -1      待ち合わせに失敗
    @retval   errno == -EAGAIN 待ち中に割り込まれた
    @retval   errno == -ENOENT 対象の子スレッドがいない
 */
int
yatos_thread_wait(tid wait_tid, thread_wait_flags wflags, tid *exit_tidp, exit_code *rcp) {
	syscall_res_type res;

	syscall4( res, SYS_YATOS_THR_WAIT, 
	    (syscall_arg_type)wait_tid, 
	    (syscall_arg_type)wflags, 
	    (syscall_arg_type)exit_tidp,
	    (syscall_arg_type)rcp);

	if ( res != 0 ) {

		set_errno(res);
		return -1;
	}

	return 0;
}
