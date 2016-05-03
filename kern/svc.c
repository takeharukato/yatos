/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  YATOS Supervisor Call routines                                    */
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

/**  ユーザ空間のイベントハンドラアドレスを登録する
     @param[in] u_evhandler ハンドラアドレス
     @retval  0       正常登録完了
     @retval -EPERM   カーネル空間から登録しようとした
     @retval -EFAULT  アドレス不正または仮想アドレス領域に実行権がない
 */
int
svc_register_common_event_handler(void *u_evhandler) {
	bool perm;

	if ( current->p == hal_refer_kernel_proc() )
		return -EPERM;
	perm = vm_user_area_can_access(&current->p->vm, 
	    u_evhandler, sizeof(void *), VMA_PROT_X);
	if ( !perm )
		return -EFAULT;

	current->p->u_evhandler = u_evhandler;

	return 0;
}

/**  自スレッドのCPUを解放する
     @retval  0       正常終了
 */
int
svc_thr_yield(void) {

	thr_yield();
	return 0;
}

/** 自スレッドを終了する
    @param[in] rc 終了コード
 */
int 
svc_thr_exit(exit_code rc) {

	thr_exit(rc);
	return 0;
}

/** 自スレッドのIDを取得する
 */
tid 
svc_thr_getid(void) {

	return current->tid;
}

/** 子スレッドの待ち合わせを行う
    @param[in]  wait_tid        待ち合わせ対象スレッドのスレッドID
    @param[in]  user_wflags     待ち合わせ対象スレッドの指定
    @param[out] user_exit_tidp  終了した子スレッドのスレッドID格納アドレス
    @param[out] user_rcp   子スレッドの終了コード格納アドレス 
    @retval    0           待ち合わせ完了
    @retval   -EAGAIN      待ち中に割り込まれた
    @retval   -ENOENT      対象の子スレッドがいない
*/
int
svc_thr_wait(tid wait_tid, thread_wait_flags user_wflags, tid *user_exit_tidp, exit_code *user_rcp) {
	int                   rc;
	thread              *thr;
	intrflags          flags;
	thread_wait_flags wflags;
	exit_code           code;
	tid              chldtid;

	wflags = ( user_wflags & 
	    ( THR_WAIT_ANY | THR_WAIT_PROC | THR_WAIT_PGRP | THR_WAIT_ID | THR_WAIT_NONBLOCK) );

	thr = thr_find_thread_by_tid(wait_tid);
	if ( ( thr == NULL ) && ( wflags & THR_WAIT_ID ) )
		return -ENOENT;

	spinlock_lock_disable_intr( &thr->lock, &flags );
	if ( ( thr->type == THR_TYPE_KERNEL) && ( wflags & THR_WAIT_ID ) ) {

		rc = -EPERM;
		goto unlock_out;
	}
	spinlock_unlock_restore_intr( &thr->lock, &flags );

	rc = thr_wait(wait_tid, wflags, &chldtid, &code);
	if ( rc != 0 )
		goto error_out;

	rc = vm_copy_out( &current->p->vm, user_rcp, (const void *)&code, sizeof(exit_code) );
	if ( rc < 0 )
		goto error_out;

	rc = vm_copy_out( &current->p->vm, user_exit_tidp, (const void *)&chldtid, sizeof(tid) );
	if ( rc < 0 )
		goto error_out;

	return 0;

unlock_out:
	spinlock_unlock_restore_intr( &thr->lock, &flags );

error_out:
	return rc;
}

/**  LPCメッセージを送信する
 メッセージを送信する
    @param[in] dest  送信先エンドポイント
    @param[in] tmout タイムアウト時間(単位:ms)
    @param[in] m     送信電文
    @retval    0     正常に送信した
 */
int
svc_lpc_send(endpoint dest, lpc_tmout tmout, void *m) {

	return lpc_send(dest, tmout, m);
}

/** メッセージを受信する
    @param[in]     src     送信元エンドポイント
    @param[in]     tmout   タイムアウト時間(単位:ms)
    @param[in]     m       受信電文格納先
    @param[in,out] msg_src 受信したメッセージの送信元エンドポイント格納先
    @retval    0       正常に受信した
    @retval   -EAGAIN  電文がなかった
    @note 
 */
int
svc_lpc_recv(endpoint src, lpc_tmout tmout, void *m, endpoint *msg_src){
	
	return lpc_recv(src, tmout, m, msg_src);
}

/** メッセージを送信し返信を待ち受ける
    @param[in] dest   送信先エンドポイント
    @param[in] m       受信電文格納先
    @retval    0       正常に受信した
    @note 送信後ユーザ空間に戻らず, リプライを受け付けるシステムコール
    典型的な送受信手順になるので, 複合システムコール(composite system call)
    として用意している。
 */
int
svc_lpc_send_and_reply(endpoint dest, void *m) {

	return lpc_send_and_reply(dest, m);
}
