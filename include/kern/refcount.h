/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Reference counter relevant definitions                            */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_REFCOUNT_H)
#define  _KERN_REFCOUNT_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/kern_types.h>
#include <kern/spinlock.h>

#define REFCNT_INITIAL_VAL          (1)  /**<  参照カウンタの初期値  */

typedef singned_cnt_type   refcnt_val;  /**<  参照カウンタの値      */

/** 参照カウンタ
 */
typedef struct _refcnt{
 	spinlock                  lock;  /**< 参照カウンタのロック                  */
	refcnt_val             counter;  /**< 参照カウンタ                          */
	bool                   deleted;  /**< 削除要求発行済み                      */
}refcnt;

/** リファレンスカウンタの初期化子
 */
#define __REFCNT_INITIALIZER {						\
		.lock = __SPINLOCK_INITIALIZER,				\
		.counter = REFCNT_INITIAL_VAL,			        \
		.deleted = false,				        \
	}

void refcnt_init(refcnt *counterp);
int  refcnt_get(refcnt *counterp, refcnt_val *valp);
int  refcnt_put(refcnt *counterp, refcnt_val *valp);
void refcnt_mark_deleted(refcnt *counterp);
#endif  /*  _KERN_REFCOUNT_H  */
