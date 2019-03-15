/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Thread  system callrelevant definitions                           */
/*                                                                    */
/**********************************************************************/
#if !defined(_ULIB_THREAD_SYSCALL_H)
#define  _ULIB_THREAD_SYSCALL_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/async-event.h>

#include <ulib/yatos-ulib.h>

void yatos_thread_yield(void);
void yatos_thread_exit(int rc);
tid  yatos_thread_getid(void);
int  yatos_thread_wait(tid _wait_tid, thread_wait_flags _wflags, tid *_exit_tidp,
    exit_code *_rcp);
int yatos_get_event_mask(event_mask *_msk);
int yatos_set_event_mask(event_mask *msk);

void ev_mask_clr(event_mask *_maskp);
bool ev_mask_test(event_mask *_mask, event_no _id);
bool ev_mask_empty(event_mask *mask);
void ev_mask_set(event_mask *_maskp, event_no _id);
void ev_mask_unset(event_mask *_maskp, event_no _id);
void ev_mask_xor(event_mask *_mask1, event_mask *_mask2, event_mask *_maskp);
void ev_mask_and(event_mask *_mask1, event_mask *_mask2, event_mask *_maskp);
void ev_mask_fill(event_mask *_mask);

#endif  /*  _ULIB_THREAD_SYSCALL_H   */
