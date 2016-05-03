/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Slab cache relevant definitions                                   */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_KMEM_CACHE_H)
#define  _KERN_KMEM_CACHE_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>
#include <kern/splay.h>
#include <kern/queue.h>

/**  kmem-cache獲得フラグ
 */
#define KM_SLEEP                     (0)  /*< メモリ解放を待ち合わせる   */
#define KM_NOSLEEP                   (1)  /*< メモリ解放を待ち合わせない */

/**    kmem-cache属性情報
 */
#define KM_ALIGN_HW                  (0)  /*< ハードウエアアラインメントに合わせる */
#define KM_LIMIT_DEFAULT             (0)  /*< デフォルトのリミットに合わせる       */

#define KM_FLAGS_DEFAULT             (0)  /*< キャッシュ獲得のデフォルト  */

#define KM_SLAB_TYPE_DIVISOR         (8)  /*< ページの1/8未満は, オンスラブキャッシュ使用  */

/**  スラブキャッシュの属性値
 */
#define KM_SLAB_ON_SLAB              (0)  /*< オンスラブキャッシュ  */
#define KM_SLAB_OFF_SLAB             (1)  /*< オフスラブキャッシュ  */
#define KM_SLAB_PREDEFINED_CACHE     (2)  /*< 規定のスラブキャッシュ  */

#define KM_STATE_DELAY_DESTROY       (1)  /*< 後でキャッシュを解放する  */

#define KM_MIN_KMALLOC_SHIFT         (3)  /*< kmallocの最小サイズ(単位:2冪 2^3 = 8 Byte) */
#define KM_LIMIT_KMALLOC_SHIFT       (18) /*< kmallocの最大サイズ(単位:2冪 2^17 =128 KiB) */

typedef struct _kmem_cache{
	spinlock                                  lock;
	const char                               *name;
	SPLAY_ENTRY(_kmem_cache)                  node;
	size_t                               color_num;
	size_t                                obj_size;
	size_t                               slab_size;
	obj_cnt_type                     objs_per_slab;
	uintptr_t                                align;
	uintptr_t                           color_next;
	slab_flags                              sflags;
	obj_cnt_type                            limits;
	obj_cnt_type                        slab_count;
	obj_cnt_type                       usage_count;
	slab_state                               state;
	queue                                  partial;
	queue                                     full;
	queue                                     free;
	void   (*constructor)(void *_obj, size_t _siz); 
	void    (*destructor)(void *_obj, size_t _siz);
}kmem_cache;

typedef struct _slab{
	list                                      link;
	queue                                  objects;
	spinlock                                  lock;
	kmem_cache                        *kmcache_ref;
	obj_cnt_type                             count;
	uintptr_t                         color_offset;
	void                                     *page;
}slab;

typedef struct _kmem_cache_db{
	spinlock lock;
	SPLAY_HEAD(kmcache_db, _kmem_cache) head;
}kmem_cache_db;

typedef struct _slab_obj_head{
	list                                      link;
}slab_obj_head;

#define __KMEM_CACHE_DB_INITIALIZER(root) {	\
         .lock = __SPINLOCK_INITIALIZER,	\
	.head = SPLAY_INITIALIZER( root ),	\
	}

#define KMEM_CACHE_INITIALIZER(entp) {			\
	.lock = __SPINLOCK_INITIALIZER,			\
	.name = "uninitilized-kmalloc-cache",	\
	.node = {((struct _kmem_cache *)(entp)), ((struct _kmem_cache *)(entp))},	\
	.color_num = 0,                         \
	.obj_size = 0,                          \
	.slab_size = 0,                         \
	.objs_per_slab = 0, 	                \
	.align = 0,                             \
	.color_next = 0,                        \
	.sflags = 0,                            \
	.limits = 0,                            \
        .slab_count = 0,                        \
        .usage_count = 0,                       \
        .state = 0,                             \
	.partial = __QUEUE_INITIALIZER( &( (struct _kmem_cache *)(entp) )->partial ), \
	.full = __QUEUE_INITIALIZER( &( (struct _kmem_cache *)(entp) )->full ), \
	.free = __QUEUE_INITIALIZER( &( (struct _kmem_cache *)(entp) )->free ), \
	 .constructor = NULL,				            \
	 .destructor = NULL,				            \
     }	

uintptr_t _kmem_calc_align_offset(kmem_cache *_kcp);
bool _kmem_is_off_slab(kmem_cache *_kcp);
slab *_kmem_obj_to_slab(void *_obj);
int _kmem_cache_grow( kmem_cache *kcp, slab_flags sflags);
kmem_cache *get_kmem_cache(const char *_name);
void put_kmem_cache(kmem_cache *_kcp);
void *kmem_cache_alloc( kmem_cache *_kcp, slab_flags _sflags );
void kmem_cache_free( kmem_cache *_kcp, void *_obj );
int kmem_cache_init(kmem_cache *_kcp, const char *_name, size_t _size, slab_flags _sflags, 
    uintptr_t _align, obj_cnt_type _limits,
    void (*_constructor)(void *, size_t), void (*_destructor)(void *, size_t));
void *kmalloc(size_t _siz, pgalloc_flags _pgflags);
void kfree(void  *_addr);
void kmalloc_cache_init(void);
#endif  /*  _KERN_KMEM_CACHE_H   */
