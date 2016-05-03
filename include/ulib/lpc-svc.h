/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Local thread communication system callrelevant definitions        */
/*                                                                    */
/**********************************************************************/
#if !defined(_ULIB_LPC_SYSCALL_H)
#define  _ULIB_LPC_SYSCALL_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <ulib/yatos-ulib.h>

int yatos_lpc_send(endpoint _dest, lpc_tmout _tmout, void *_m);
int yatos_lpc_recv(endpoint _src, lpc_tmout _tmout, void *_m, endpoint *_sender);
int yatos_lpc_send_and_reply(endpoint _dest, void *_m);
#endif  /*  _ULIB_LPC_SYSCALL_H   */
