/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  mutex relevant definitions                                        */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_MUTEX_H)
#define  _KERN_MUTEX_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>
#include <kern/thread-sync.h>

#define MTX_FLAG_EXCLUSIVE  (0)  /*<  自己再入不可能mutex  */
#define MTX_FLAG_RECURSIVE  (1)  /*<  自己再入可能mutex  */

struct _thread;
typedef struct _mutex{
	spinlock         lock;
	sync_obj mutex_waiter;
	mutex_flags mtx_flags;
	obj_cnt_type  counter;
	struct _thread *owner;
}mutex;

void mutex_init(mutex *_mtx, mutex_flags _mtx_flags);
void mutex_destroy(mutex *_mtx);
bool mutex_lock(mutex *_mtx);
void mutex_unlock(mutex *_mtx);
bool mutex_locked_by_self(mutex *_mtx);
#endif  /*  _KERN_MUTEX_H   */
