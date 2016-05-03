/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  ID bitmap relevant definitions                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_ID_BITMAP_H)
#define  _KERN_ID_BITMAP_H 

#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>

#define ID_FLAG_USER   (0)  /*< ユーザ資源を取得    */
#define ID_FLAG_SYSTEM (1)  /*< システム資源を取得  */

typedef struct _id_bitmap{
	spinlock lock;
	uint64_t map[ID_BITMAP_ARRAY_IDX];
}id_bitmap;

#define __ID_BITMAP_INITIALIZER					\
	{							\
	.lock=__SPINLOCK_INITIALIZER,			        \
	.map = {0,},				                \
	}

void init_id_bitmap(id_bitmap *_idmap);
int get_specified_id(id_bitmap *idmap, obj_id id, int flags);
int get_id(id_bitmap *_idmap, obj_id *_idp, int flags);
void put_id(id_bitmap *_idmap, obj_id _id);
bool is_id_busy(id_bitmap *_idmap, obj_id _id);
#endif  /*  _KERN_ID_BITMAP_H   */
