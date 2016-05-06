/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Thread state relevant definitions                                 */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_THR_STATE_H)
#define  _KERN_THR_STATE_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>

/** スレッドの状態
 */
typedef enum _thr_state{
	THR_TSTATE_FREE=0,           /*< スレッド未使用状態        */
	THR_TSTATE_DORMANT=1,        /*< スレッド停止中            */
	THR_TSTATE_READY=2,          /*< 実行可能状態              */
	THR_TSTATE_RUN=3,            /*< 実行中                    */
	THR_TSTATE_WAIT=4,           /*< 資源待ち中                */
	THR_TSTATE_WAIT_KILLABLE=5,  /*< 資源待ち中(強制終了可能)  */
	THR_TSTATE_WAIT_INTR=6,      /*< 資源待ち中(割込み可能)    */
	THR_TSTATE_EXIT=7,           /*< 終了, 回収待ち            */
}thr_state;

/** スレッド状態が資源待ちであることを確認する
 */
#define valid_wait_status(status)			\
	( ( (status) == THR_TSTATE_WAIT )           ||	\
	  ( (status) == THR_TSTATE_WAIT_KILLABLE )  ||	\
	  ( (status) == THR_TSTATE_WAIT_INTR ) )

/** スレッドが強制終了可能であることを確認する
 */
#define thr_wait_killable(thr)				\
	( ( (thr)->status ) == THR_TSTATE_WAIT_KILLABLE )

/** スレッドが割込み可能な休眠状態にあることを確認する
 */
#define thr_wait_intr(thr)					\
	(  thr_wait_killable(thr)                     ||	\
	    ( ((thr)->status) == THR_TSTATE_WAIT_INTR ) )
	
/** スレッドが休眠状態にあることを確認する
 */

#define thr_in_wait(thr)					\
	(   ( ( (thr)->status ) == THR_TSTATE_WAIT )  ||	\
	    ( thr_wait_intr(thr) ) )

#endif  /*  _KERN_THR_STATE_H   */
