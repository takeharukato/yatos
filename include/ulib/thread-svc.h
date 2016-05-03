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

#include <ulib/yatos-ulib.h>

void yatos_thread_yield(void);
void yatos_thread_exit(int rc);
tid  yatos_thread_getid(void);
int  yatos_thread_wait(tid _wait_tid, thread_wait_flags _wflags, tid *_exit_tidp, exit_code *_rcp);
#endif  /*  _ULIB_THREAD_SYSCALL_H   */
