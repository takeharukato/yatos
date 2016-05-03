/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Thread module internal definitions                                */
/*  Note: Following definitions must be used in a logical module only */
/*                                                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(__THR_INTERNAL_H)
#define  __THR_INTERNAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>

struct _thread;
struct _sync_obj;
struct _sync_block;
enum _sync_reason;
void _thr_init_kthread_params(struct _thread *_thr);
void _ti_set_ti_with_thread(struct _thread *_thr);
void _thr_init_reaper(void);
void _sched_wakeup(struct _thread *_thr);
void _sync_init_block(struct _sync_block *_blk);
void _thr_enter_dead(void);
void _thr_do_idle(void);
void _sync_wait_no_schedule(struct _sync_obj *_obj, struct _sync_block *_blkp);
enum _sync_reason _sync_finish_wait(sync_obj *_obj, sync_block *_blk);
#endif  /*  __THR_INTERNAL_H   */
