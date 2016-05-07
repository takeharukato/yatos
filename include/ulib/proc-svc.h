/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  process system call relevant definitions                          */
/*                                                                    */
/**********************************************************************/
#if !defined(_ULIB_PROC_SVC_H)
#define  _ULIB_PROC_SVC_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <ulib/yatos-ulib.h>

int yatos_proc_send_event(tid _dest, event_id  _id, event_data _data);
int yatos_proc_send_proc_event(tid _dest, event_id  _id, event_data _data);
int yatos_proc_bcast_proc_event(tid _dest, event_id  _id, event_data _data);
int yatos_proc_create_thread(thr_prio _prio, int (*_start)(void *), void *_arg, 
    void *_sp, tid *_newidp);
int yatos_proc_get_thread_resource(tid _id, thread_resource *_resp);
#endif  /*  _ULIB_PROC_SVC_H   */
