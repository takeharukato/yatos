/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  relevant definitions                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_THREAD_RESOURCE_H)
#define  _KERN_THREAD_RESOURCE_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>

/** スレッド消費資源
 */
typedef struct _thread_resource{
	ticks            sys_time;  /*< システム時間              */
	ticks           user_time;  /*< ユーザ時間                */
	ticks   children_sys_time;  /*< 子スレッドのシステム時間  */
	ticks  children_user_time;  /*< 子スレッドのユーザ時間    */
}thread_resource;
#endif  /*  _KERN_THREAD_RESOURCE_H   */
