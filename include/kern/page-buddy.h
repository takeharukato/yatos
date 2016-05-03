/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  buddy page allocator relevant definitions                         */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_BUDDY_PAGE_H)
#define  _KERN_BUDDY_PAGE_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/queue.h>

struct _page_frame;
typedef struct _page_buddy{
	spinlock                               lock;
	struct _page_frame                   *array;
	obj_cnt_type                      start_pfn;
	obj_cnt_type                       nr_pages;
	obj_cnt_type   free_nr[PAGE_POOL_MAX_ORDER];
	queue        page_list[PAGE_POOL_MAX_ORDER];
}page_buddy;

void page_buddy_enqueue(page_buddy *_buddy, obj_cnt_type _pfn);
int page_buddy_dequeue(page_buddy *_buddy, page_order _order, obj_cnt_type *_pfnp);
obj_cnt_type page_buddy_get_free(page_buddy *_buddy);
void page_buddy_init(page_buddy *_buddy, struct _page_frame *_array, obj_cnt_type _start_pfn, 
    obj_cnt_type _nr_pfn);
#endif  /*  _KERN_BUDDY_PAGE_H   */
