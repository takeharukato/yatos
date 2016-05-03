/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2014 Takeharu KATO                                      */
/*                                                                    */
/*  kmalloc/kfree(predefined kmem-caches) routines                    */
/*                                                                    */
/**********************************************************************/
#include <stddef.h>
#include <stdint.h>

#include <kern/config.h>
#include <kern/assert.h>
#include <kern/string.h>
#include <kern/splay.h>
#include <kern/spinlock.h>
#include <kern/queue.h>
#include <kern/page-kmcache.h>
#include <kern/page.h>
#include <kern/errno.h>

#define KMALLOC_NR (KM_LIMIT_KMALLOC_SHIFT - KM_MIN_KMALLOC_SHIFT)

typedef struct _kmalloc_def{
	size_t       siz;
	const char *name;
}kmalloc_def;

static kmalloc_def kmalloc_dic[] = {
	{ ( 1 << KM_MIN_KMALLOC_SHIFT ), "kmalloc-8"},
	{ ( 1 << ( KM_MIN_KMALLOC_SHIFT + 1) ), "kmalloc-16"},
	{ ( 1 << ( KM_MIN_KMALLOC_SHIFT + 2) ), "kmalloc-32"},
	{ ( 1 << ( KM_MIN_KMALLOC_SHIFT + 3) ), "kmalloc-64"},
	{ ( 1 << ( KM_MIN_KMALLOC_SHIFT + 4) ), "kmalloc-128"},
	{ ( 1 << ( KM_MIN_KMALLOC_SHIFT + 5) ), "kmalloc-256"},
	{ ( 1 << ( KM_MIN_KMALLOC_SHIFT + 6) ), "kmalloc-512"},
	{ ( 1 << ( KM_MIN_KMALLOC_SHIFT + 7) ), "kmalloc-1024"},
	{ ( 1 << ( KM_MIN_KMALLOC_SHIFT + 8) ), "kmalloc-2048"},
	{ ( 1 << ( KM_MIN_KMALLOC_SHIFT + 9) ), "kmalloc-4096"},
	{ ( 1 << ( KM_MIN_KMALLOC_SHIFT + 10) ), "kmalloc-8192"},
	{ ( 1 << ( KM_MIN_KMALLOC_SHIFT + 11) ), "kmalloc-16384"},
	{ ( 1 << ( KM_MIN_KMALLOC_SHIFT + 12) ), "kmalloc-32768"},
	{ ( 1 << ( KM_MIN_KMALLOC_SHIFT + 13) ), "kmalloc-65536"},
	{ ( 1 << ( KM_MIN_KMALLOC_SHIFT + 14) ), "kmalloc-131072"},
	{ 0, NULL},
};

static kmem_cache kmalloc_caches[KMALLOC_NR] = {
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[0]),
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[1]),
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[2]),
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[3]),
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[4]),
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[5]),
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[6]),
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[7]),
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[8]),
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[9]),
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[10]),
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[11]),
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[12]),
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[13]),
	KMEM_CACHE_INITIALIZER(&kmalloc_caches[14]),
};

/** 所定のサイズのkmallocエントリを算出する
    @param[in] siz 要求メモリ獲得サイズ
    @return    要求されたメモリを割当てるkmem_cache情報のアドレス
 */
static kmem_cache *
find_kmalloc_ent(size_t siz) {
	int idx, kcp_idx;
	size_t cache_siz;

	for( idx = 0; idx < KMALLOC_NR; ++idx) {

		kcp_idx = idx + KM_MIN_KMALLOC_SHIFT;
		cache_siz = ( (size_t)1 ) << kcp_idx ;
		if ( siz  <=  cache_siz )
			goto success_out;
	}

	goto error_out;

success_out:
	kassert( siz <= cache_siz );
 	kassert( idx < KMALLOC_NR );

	return &kmalloc_caches[idx];

error_out:
	return NULL;
}

/** ページサイズのべき乗になっていないメモリを獲得する
    @param[in] siz    獲得メモリサイズ(単位:バイト)
    @param[in] sflags メモリ獲得条件
    @return 非NULL    獲得したメモリ領域
    @return NULL      メモリ不足によりメモリが獲得できなかった
 */
void *
kmalloc(size_t siz, pgalloc_flags __attribute__ ((unused)) pgflags) {
	kmem_cache *kcp;
	kmem_cache *ref;
	void       *obj;

	kcp = find_kmalloc_ent(siz);
	if (kcp == NULL)
		return NULL;

	ref = get_kmem_cache(kcp->name);
	kassert( ref == kcp );

	obj = kmem_cache_alloc( kcp, 0 );

	put_kmem_cache(kcp);

	return obj;
}

/** ページサイズのべき乗になっていないメモリを開放する
    @param[in] obj 開放対象のメモリ領域
 */
void
kfree(void *obj) {
	kmem_cache *kcp;
	kmem_cache *ref;
	slab     *slabp;

	if ( obj == NULL )
		return;
	
	slabp = _kmem_obj_to_slab(obj);

	kcp = slabp->kmcache_ref;

	ref = get_kmem_cache(kcp->name);
	kassert( ref == kcp );

	kmem_cache_free( kcp, obj );

	put_kmem_cache(kcp);
}

/** kmallocキャッシュの初期化
 */
void
kmalloc_cache_init(void) {
	int             i;
	int            rc;
	kmalloc_def  *ref;
	kmem_cache   *kcp;

	for(i = 0; i < KMALLOC_NR; ++i) {
		
		ref = &kmalloc_dic[i];
		if (ref->name == NULL)
			break;

		kcp = &kmalloc_caches[i];
		kmem_cache_init(kcp, ref->name, ref->siz, 
		    ( KM_SLAB_ON_SLAB | KM_SLAB_PREDEFINED_CACHE ), 
		    KM_ALIGN_HW, KM_LIMIT_DEFAULT, 
		    NULL, NULL);

		
		if ( !_kmem_is_off_slab(kcp) ) {
			
			rc = _kmem_cache_grow( kcp, 0);
			kassert(rc == 0);
		}
	}
}
