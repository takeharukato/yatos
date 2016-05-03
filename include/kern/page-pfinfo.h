/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Page Frame Data Base relevant definitions                         */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_PFINFO_H)
#define  _KERN_PFINFO_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>
#include <kern/list.h>
#include <kern/queue.h>
#include <kern/page-buddy.h>

struct _page_frame;

/** ページフレーム管理情報
 */
typedef struct _page_frame_info{
	spinlock             lock;  /*< ページフレーム管理情報のロック                */
	list                 link;  /*< ページフレーム管理情報のリンク                */
	obj_cnt_type      min_pfn;  /*< 最小ページフレーム番号                        */
	obj_cnt_type      max_pfn;  /*< 最大ページフレーム番号                        */
	obj_cnt_type     nr_pages;  /*< ページフレーム管理情報の管理対象ページ数
				     * (ページフレーム配列を除く)  
				     */
	struct _page_frame *array;  /*< ページフレーム配列                            */
	page_buddy          buddy;  /*< バディ管理情報                                */
	private_inf       private;  /*< アーキ依存情報                                */
}page_frame_info;

/** ページフレーム管理情報キュー
 */
typedef struct _page_frame_queue{
	spinlock   lock;           /*< ページフレーム管理情報キューのロック           */
	queue       que;           /*< ページフレーム管理情報キュー                   */
}page_frame_queue;

/** ページフレーム管理情報キューの初期化子
 */
#define __PF_QUEUE_INITIALIZER(pfque) {		\
	.lock = __SPINLOCK_INITIALIZER,		\
	.que  = __QUEUE_INITIALIZER(pfque),	\
	}
#endif  /*  _KERN_PFINFO_H   */
