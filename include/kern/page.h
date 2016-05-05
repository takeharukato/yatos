/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  page pool relevant definitions                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_PAGE_H)
#define  _KERN_PAGE_H 

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
#include <kern/page-pfinfo.h>
#include <kern/page-buddy.h>
#include <kern/page-kmcache.h>
#include <hal/arch-page.h>

/** ページの状態(共通部)
 */
#define PAGE_CSTATE_FREE     (0x0)  /*< 未使用  */
#define PAGE_CSTATE_USED     (0x1)  /*< 使用中  */
#define PAGE_CSTATE_RESERVED (0x2)  /*< カーネルやメモリマップトデバイスが予約  */

#define PAGE_CSTATE_NOT_FREED(pg) \
	( ( ( (struct _page_frame *)(pg) )->state ) & ( PAGE_CSTATE_USED | PAGE_CSTATE_RESERVED ) )


#define KMALLOC_NORMAL       (0)  /*< 通常獲得        */
#define KMALLOC_ATOMIC       (1)  /*< アトミック獲得  */

struct _slab;
struct _page_frame_info;
struct _page_buddy;
typedef struct _page_frame{
	spinlock                lock;
	list                    link;
	page_state             state;
	page_state        arch_state;
	obj_cnt_type             pfn;
	obj_cnt_type          mapcnt;	
	page_order             order;
	struct _page_buddy   *buddyp;
	struct _slab          *slabp;
}page_frame;

void kcom_add_page_info(struct _page_frame_info *_pfi);
int alloc_buddy_pages(void **_addrp, page_order _order, pgalloc_flags _pgflags);
void free_buddy_pages(void *_addr);
int get_free_page(void **_addrp);
int free_page(void *_addr);

void inc_page_map_count(void *addrp);
void dec_page_map_count(void *addrp);
void calc_page_order(size_t _size, page_order *_res);

int kvaddr_to_page_frame(void *addrp, page_frame  **pp);
int pfn_to_page_frame(obj_cnt_type _pfn, page_frame  **_pp);

bool kcom_is_pfn_valid_nolock(obj_cnt_type _pfn);
bool kcom_is_pfn_valid(obj_cnt_type _pfn);

int hal_pfn_to_kvaddr(obj_cnt_type _pfn, void **_kvaddrp);
int hal_kvaddr_to_pfn(void *_kvaddr, obj_cnt_type *_pfnp);
bool hal_is_pfn_reserved(obj_cnt_type _pfn);
#endif  /*  _KERN_PAGE_H   */
