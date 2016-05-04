/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  fake spinlock definitions                                         */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_SPINLOCK_H)
#define  _KERN_SPINLOCK_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/backtrace.h>

#define SPINLOCK_TYPE_NORMAL     (0x0)  /*< Non recursive lock  */
#define SPINLOCK_TYPE_RECURSIVE  (0x1)  /*< Recursive lock      */

struct _thread_info;
typedef struct _spinlock {
	uint32_t            locked;  /*  Is the lock held?                                */
	uint32_t              type;  /*  lock type                                        */
	uint32_t               cpu;  /*  The cpu holding the lock.                        */
	uint32_t             depth;  /*  lock depth                                       */
	struct _thread_info *owner;  /*  lock owner thread info                           */
	uintptr_t backtrace[SPINLOCK_BT_DEPTH];       /*  back trace for debug            */
}spinlock;

#define __SPINLOCK_INITIALIZER		 \
	{				 \
		.locked = 0,		 \
		.type  = SPINLOCK_TYPE_NORMAL , \
		.cpu    = 0,             \
		.depth  = 0,             \
		.owner  = NULL,          \
	}

void spinlock_init(spinlock *_lock);
void spinlock_lock(spinlock *_lock);
void spinlock_unlock(spinlock *_lock);
void raw_spinlock_lock_disable_intr(spinlock *_lock, intrflags *_flags);
void raw_spinlock_unlock_restore_intr(spinlock *_lock, intrflags *_flags);
void spinlock_lock_disable_intr(spinlock *_lock, intrflags *_flags);
void spinlock_unlock_restore_intr(spinlock *_lock, intrflags *_flags);
bool spinlock_locked_by_self(spinlock *_lock);
bool check_recursive_locked(spinlock *_lock);

void hal_spinlock_lock(spinlock *_lock);
void hal_spinlock_unlock(spinlock *_lock);

void hal_cpu_disable_interrupt(intrflags *_flags);
void hal_cpu_restore_interrupt(intrflags *_flags);
void hal_cpu_enable_interrupt(void);
bool hal_cpu_interrupt_disabled(void);
#endif  /*  _KERN_SPINLOCK_H   */
