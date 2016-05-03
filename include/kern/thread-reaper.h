/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Thread reaper relevant definitions                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_THREAD_REAPER_H)
#define  _KERN_THREAD_REAPER_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/thread-que.h>
#include <kern/thread-sync.h>

#include <thr/thr-internal.h>

struct _thread;
/** スレッド回収処理関連データ構造
 */
typedef struct _thread_reaper{
	struct _thread *thr;  /*<  回収スレッド                        */
	sync_obj        wai;  /*<  終了スレッド待ち合わせオブジェクト  */
	thread_queue     tq;  /*<  終了待ちスレッドのキュー            */
}thread_reaper;

#endif  /*  _KERN_THREAD_REAPER_H   */
