/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  thread relevant definitions                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_THREAD_H)
#define  _KERN_THREAD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/align.h>
#include <kern/spinlock.h>
#include <kern/queue.h>
#include <kern/list.h>
#include <kern/rbtree.h>
#include <kern/thread-info.h>
#include <kern/thread-que.h>
#include <kern/thread-sync.h>
#include <kern/thread-reaper.h>
#include <kern/thread-resource.h>
#include <kern/async-event.h>
#include <kern/lpc.h>
#include <kern/kresv-ids.h>

#include <hal/arch-cpu.h> 

//#define ENABLE_JOINABLE_KTHREAD  /*< tst-wait-kthread.cのTPを実行する場合は有効にする */

/** スレッドの状態
 */
typedef enum _thr_state{
	THR_TSTATE_FREE=0,     /*< スレッド未使用状態          */
	THR_TSTATE_DORMANT=1,  /*< スレッド停止中              */
	THR_TSTATE_READY=2,    /*< 実行可能状態                */
	THR_TSTATE_RUN=3,      /*< 実行中                      */
	THR_TSTATE_WAIT=4,     /*< 資源待ち中                  */
	THR_TSTATE_EXIT=5,     /*< 終了, 回収待ち              */
}thr_state;

/** スレッド種別
 */
typedef enum _thread_type{
	THR_TYPE_NONE=0,    /*< 無属性スレッド    */
	THR_TYPE_KERNEL=1,  /*< カーネルスレッド  */
	THR_TYPE_USER=2,    /*< ユーザスレッド    */
}thread_type;

/** スレッド属性
 */
#define THR_FLAG_NONE     (0x0)     /*< 無属性                          */ 
#define THR_FLAG_JOINABLE (0x1)     /*< 親スレッドのWAITを待ち合わせる  */

/** 終了待ち合わせフラグ
 */
#define THR_WAIT_ANY      (0x0)     /*< 無条件待ち合わせ                                 */
#define THR_WAIT_PROC     (0x1)     /*< 同一プロセス内のスレッドを待ち合わせる           */
#define THR_WAIT_PGRP     (0x2)     /*< 同一プロセスグループ内のスレッドを待ち合わせる
				     *  (未実装)
				     */
#define THR_WAIT_ID       (0x4)     /*< 指定されたスレッドIDを待ち合わせる
				     * THR_WAIT_PROC, THR_WAIT_PGRPは無視される
				     */
#define THR_WAIT_NONBLOCK (0x10)    /*< ノンブロックで待ち合わせる  */  
/** スレッドID
 */
#define THR_IDLE_TID       (ID_RESV_IDLE)    /*< アイドルスレッド/カーネルプロセスのtid  */
#define THR_INVALID_TID    (ID_RESV_INVALID) /*< 未初期化スレッドのtid                   */

/** ラウンドロビンスケジューリング優先度
 */
#define THR_RR_PRIO      (0)

struct _thread_queue;
struct _proc;

/** スレッド管理情報
    @note スレッドのロックとキューのロックを同時に取る際は, キューのロックを
    先に取ること。
 */
typedef struct _thread{
	spinlock                   lock;  /*< ロック変数                                    */
	thr_state                status;  /*< スレッド状態                                  */
	RB_ENTRY(_thread)         mnode;  /*< スレッド一覧の赤黒木のノード                  */
	list                      plink;  /*< プロセスへのリンク                            */
	list                       link;  /*< キューへのリンク                              */
	list                parent_link;  /*< 子スレッドキューへのリンク                    */
	exit_code             exit_code;  /*< 終了コード                                    */
	struct _thread_info         *ti;  /*< スレッド情報へのポインタ                      */
	struct _proc                 *p;  /*< 所属するプロセス空間へのポインタ              */
	tid                         tid;  /*< スレッドID                                    */
	tid                        ptid;  /*< 親スレッドID                                  */
	sync_obj          children_wait;  /*< 子スレッド待ち合わせ同期オブジェクト          */
	sync_obj            parent_wait;  /*< 親スレッド待ち合わせ同期オブジェクト          */
	queue              exit_waiters;  /*< 終了待ち合わせ子スレッド群                    */
	queue                  children;  /*< 子スレッド群                                  */
	thread_resource        resource;  /*< スレッド消費資源                              */
	thr_prio                   prio;  /*< スレッドの静的優先度                          */
	thr_prio                  slice;  /*< スレッドのタイムスライス                      */
	thr_prio              cur_slice;  /*< 現在のタイムスライス                          */
	thread_flags          thr_flags;  /*< スレッドの属性コード                          */
	thread_type                type;  /*< スレッド種別                                  */
	kstack_type                 ksp;  /*< カーネルスタックの先頭アドレス                */
	kstack_type            last_ksp;  /*< 最後にディスパッチしたときのスタックポインタ  */
	fpu_context               fpctx;  /*< FPUのコンテキスト情報                         */
	msg_queue                  mque;  /*< メッセージキュー                              */
	event_queue               evque;  /*< イベントキュー                                */
}thread;

/** スレッド管理用辞書
 */
typedef struct _thread_dic{
	spinlock                            lock;  /*< 辞書排他用のロック    */
	RB_HEAD(thread_id_tree, _thread) booking;  /*< スレッド一覧のヘッド  */
}thread_dic;

#define __THREAD_DIC_INITIALIZER(root)  {	\
	.lock =  __SPINLOCK_INITIALIZER,	\
	.booking = RB_INITIALIZER(root),        \
	}

void thr_idpool_init(void);
/*   IF関数  */
thread *thr_find_thread_by_tid(tid _key);
int  thr_new_thread(thread **_thrp);
int  thr_create_kthread(thread *_thr, int _prio, thread_flags _thr_flags, tid _newid, 
    int (*_fn)(void *), void *_arg);
int  thr_create_uthread(thread *_thr, int _prio, thread_flags _thr_flags, struct _proc *_p, 
    void *_ustart, uintptr_t _arg1, uintptr_t _arg2, uintptr_t _arg3, void *_ustack);
int  thr_start(thread *_thr, tid _ptid);
void thr_exit(exit_code _rc);
int  thr_destroy(thread *_thr);
void thr_yield(void);
int thr_wait(tid _tid, thread_wait_flags _wflags, tid *exit_tidp, exit_code *_rcp);

/*  HAL->共通部 IF */
void kcom_launch_new_thread(int (*_start)(void *), void *_arg);

/*  アーキ依存部  */
struct _proc *hal_refer_kernel_proc(void);
void hal_fpctx_init(fpu_context *_fpctx);
void hal_fpu_context_switch(struct _thread *_prev, struct _thread *_next);
void hal_do_context_switch(void **_prev_stkp, void **_next_stkp);
void hal_setup_kthread_function(struct _thread *_thr, int (*_fn)(void *), void *_arg);
int hal_setup_uthread_kstack(void *_start, uintptr_t _arg1, uintptr_t _arg2, 
    uintptr_t _arg3, void *_usp, void *_kstack, void **_spp);
void hal_set_exception_stack(void *_ksp);
void hal_switch_address_space(struct _proc *_prev, struct _proc *_next);
#endif  /*  _KERN_THREAD_H   */
