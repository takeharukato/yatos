/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  relevant definitions                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_ULIB_YATOS_ULIB_H)
#define  _ULIB_YATOS_ULIB_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/errno.h>
#include <kern/kern_types.h>
#include <kern/string.h>
#include <kern/kresv-ids.h>
#include <kern/svc.h>
#include <kern/thread-resource.h>
#include <kern/thread.h>
#include <kern/messages.h>
#include <kern/kname-service.h>

#include <hal/syscall-macros.h>

#include <ulib/utils.h>

extern char **environ;
int __errno(void);
void set_errno(syscall_res_type _res);
void _clear_bss(void);
#endif  /*  _ULIB_YATOS_ULIB_H   */
