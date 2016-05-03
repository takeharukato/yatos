/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  System Call relevant definitions                                  */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_SVC_H)
#define  _KERN_SVC_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>

#define SYS_YATOS_MIN_NOSYS          (0)
#define SYS_YATOS_EV_REG_EVHANDLER   (1)
#define SYS_YATOS_EV_RETURN          (2)
#define SYS_YATOS_THR_YIELD          (3)
#define SYS_YATOS_THR_EXIT           (4)
#define SYS_YATOS_THR_GETID          (5)
#define SYS_YATOS_THR_WAIT           (6)
#define SYS_YATOS_LPC_SEND           (7)
#define SYS_YATOS_LPC_RECV           (8)
#define SYS_YATOS_LPC_SEND_AND_REPLY (9)
#define SYS_YATOS_MAX_NOSYS          (10)

int svc_register_common_event_handler(void *_u_evhandler);
int svc_thr_yield(void);
int svc_thr_exit(exit_code _rc);
tid svc_thr_getid(void);
int svc_thr_wait(tid _wait_tid, thread_wait_flags _user_wflags, tid *_user_exit_tidp, exit_code *_user_rcp);
int svc_lpc_send(endpoint _dest, lpc_tmout _tmout, void *_m);
int svc_lpc_recv(endpoint _src, lpc_tmout _tmout, void *_m, endpoint *_msg_src);
int svc_lpc_send_and_reply(endpoint _dest, void *_m);
#endif  /*  _KERN_SVC_H   */
