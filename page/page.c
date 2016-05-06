/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  page routines                                                     */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/errno.h>
#include <kern/spinlock.h>
#include <kern/page.h>
#include <kern/queue.h>
#include <kern/vm.h>

static page_frame_queue global_pfque = __PF_QUEUE_INITIALIZER( &global_pfque.que );

/** 総メモリ量と空きメモリ量を取得する
    @param[in] nr_pages_p      総メモリ量返却先（単位:ページ数)
    @param[in] nr_free_pages_p 空きメモリ量返却先（単位:ページ数)
 */
void
kcom_refer_free_pages(obj_cnt_type *nr_pages_p, obj_cnt_type *nr_free_pages_p) {
	page_frame_info   *pfi;
	uint64_t nr_free_pages;
	uint64_t      nr_pages;
	list               *li;
	intrflags        flags;

	nr_free_pages = 0;
	nr_pages = 0;

	spinlock_lock_disable_intr(&global_pfque.lock, &flags);

	for( li = queue_ref_top( &global_pfque.que );
	     li != (list *)&global_pfque.que;
	     li = li->next ) {

		pfi = CONTAINER_OF(li, page_frame_info, link);
		nr_pages += pfi->nr_pages;
		nr_free_pages += page_buddy_get_free( &pfi->buddy );
	}

	spinlock_unlock_restore_intr(&global_pfque.lock, &flags);

	*nr_pages_p = nr_pages;
	*nr_free_pages_p = nr_free_pages;
}


/** ページフレーム番号から対応するページフレーム管理情報を探査する(実処理部)
    @param[in] pfn   探索するページフレーム番号
    @param[in] pfip ページ領域のアドレスを返却するアドレス
    @retval  0       ページ領域を見つけた
    @retval -ENOENT  管理対象外のページフレーム番号を指定した
    @note  HALなどから呼ばれる
 */
static int
_pfn_to_page_frame_info_nolock(obj_cnt_type pfn, page_frame_info **pfip) {
	int               rc;
	list             *li;
	page_frame_info *pfi;

	kassert( spinlock_locked_by_self(&global_pfque.lock) );	
	kassert( pfip != NULL );

	for( li = queue_ref_top( &global_pfque.que );
	     li != (list *)&global_pfque.que;
	     li = li->next ) {

		pfi = CONTAINER_OF(li, page_frame_info, link);
		if ( ( pfi->min_pfn <= pfn ) && ( pfn < pfi->max_pfn ) ) {
			
			*pfip = pfi;	
			rc = 0;
			goto unlock_out;
		}
	}

	rc = -ENOENT;

unlock_out:

	return rc;
}

/** ページフレーム番号から対応するページフレーム管理情報を探査する
    @param[in] pfn   探索するページフレーム番号
    @param[in] pfip ページ領域のアドレスを返却するアドレス
    @retval  0       ページ領域を見つけた
    @retval -ENOENT  管理対象外のページフレーム番号を指定した
 */
static int
pfn_to_page_frame_info(obj_cnt_type pfn, page_frame_info **pfip) {
	int           rc;
	intrflags  flags;

	kassert( pfip != NULL );

	spinlock_lock_disable_intr(&global_pfque.lock, &flags);
	rc = _pfn_to_page_frame_info_nolock(pfn, pfip);
	spinlock_unlock_restore_intr(&global_pfque.lock, &flags);

	return rc;
}

/** ページ情報を追加し, 初期化する
    @param[in] pfi HALから渡されたメモリ領域のページフレーム管理情報
 */
void
kcom_add_page_info(page_frame_info *pfi) {
	obj_cnt_type   i;
	page_frame    *p;
	intrflags  flags;

	kassert( pfi != NULL );

        /*
	 * ページフレーム管理情報の初期化
	 */
	spinlock_init( &pfi->lock );
	list_init( &pfi->link );

	/*
	 * バディプールの管理情報の初期化
	 */
	page_buddy_init(&pfi->buddy, pfi->array, pfi->min_pfn, 
	    pfi->min_pfn + pfi->nr_pages);

	/*
	 * ページアレイの初期化
	 */
	for(i = 0; i < pfi->nr_pages; ++i) {
		
		p = &pfi->array[i];

		spinlock_init(&p->lock);
		list_init(&p->link);
		p->pfn = i + pfi->min_pfn;
		p->mapcnt = 0;
		p->order = 0;
		p->buddyp = &pfi->buddy;
		p->slabp = NULL;
		p->arch_state = 0;

		p->state = PAGE_CSTATE_RESERVED;

		if ( ( p->pfn < pfi->max_pfn ) && ( !hal_is_pfn_reserved(p->pfn) ) ) 
			p->state = PAGE_CSTATE_USED; /*  予約領域はバディに追加しない  */
	}

        /*
	 * ページフレーム管理情報の登録
	 */
	spinlock_lock_disable_intr(&global_pfque.lock, &flags);
	queue_add( &global_pfque.que, &pfi->link );
	spinlock_unlock_restore_intr(&global_pfque.lock, &flags);

	/*
	 * バディにべージを追加
	 */	
	spinlock_lock_disable_intr(&pfi->buddy.lock, &flags);
	for(i = 0; i < pfi->nr_pages; ++i)  {

		p = &pfi->array[i];
		if ( !( p->state & PAGE_CSTATE_RESERVED ) )


			page_buddy_enqueue(&pfi->buddy, p->pfn); 
	}
	spinlock_unlock_restore_intr(&pfi->buddy.lock, &flags);
}

/** ページの予約を解除する
    @param[in] pfn ページフレーム番号
    @retval    0      正常に解除した
    @retval   -EINVAL 予約ページでない
    @retval   -ENOENT ページフレーム番号が不正
 */
int
page_release_reservation(obj_cnt_type pfn) {
	int               rc;
	page_frame     *page;
	intrflags      flags;

	rc = pfn_to_page_frame(pfn, &page);
	if ( rc != 0 )
		return rc;
	kassert( page != NULL );

	if ( !( page->state & PAGE_CSTATE_RESERVED ) )
		return -EINVAL;
	
	page->state &= ~PAGE_CSTATE_RESERVED;
	kassert( page->buddyp != NULL );

	spinlock_lock_disable_intr(&page->buddyp->lock, &flags);
	page_buddy_enqueue( page->buddyp, page->pfn); 
	spinlock_unlock_restore_intr(&page->buddyp->lock, &flags);	
	
	return 0;
}

/** ページフレーム番号からページフレーム情報を取得する
    @param[in] pfn   ページフレーム番号
    @param[in] pp    ページフレーム情報を指すポインタのアドレス
    @retval    0      ページフレーム情報を正常に返却した
    @retval   -ENOENT 指定されたページフレーム番号は利用可能な物理ページではない
 */
int
pfn_to_page_frame(obj_cnt_type pfn, page_frame  **pp) {
	int               rc;
	page_frame_info *pfi;
	
	kassert( pp != NULL );
	
	rc = pfn_to_page_frame_info(pfn, &pfi);
	if ( rc != 0 )
		return rc;

	*pp = &pfi->array[ pfn - pfi->min_pfn ];

	return 0;
}

/** ページフレーム番号に対応するページが登録されていることを確認する(ロック無し)
    @param[in] pfn    探索するページフレーム番号
    @retval    true   ページフレーム番号に対応するページが登録されている
    @retval    false  ページフレーム番号に対応するページが登録されていない
 */
bool
kcom_is_pfn_valid_nolock(obj_cnt_type pfn) {
	int               rc;
	page_frame_info *pfi;
	
	rc = _pfn_to_page_frame_info_nolock(pfn, &pfi);

	return ( rc == 0 );
}

/** ページフレーム番号に対応するページが登録されていることを確認する
    @param[in] pfn    探索するページフレーム番号
    @retval    true   ページフレーム番号に対応するページが登録されている
    @retval    false  ページフレーム番号に対応するページが登録されていない
 */
bool
kcom_is_pfn_valid(obj_cnt_type pfn) {
	int               rc;
	page_frame_info *pfi;
	
	rc = pfn_to_page_frame_info(pfn, &pfi);

	return ( rc == 0 );
}

/** カーネルアドレスからページフレーム情報を取得する
    @param[in] addrp  カーネルアドレス
    @param[in] pp     ページフレーム情報を指すポインタのアドレス
    @retval    0      ページフレーム情報を正常に返却した
    @retval   -ENOENT 指定されたアドレスは利用可能な物理ページではない
 */
int
kvaddr_to_page_frame(void *addrp, page_frame  **pp) {
	int               rc;
	page_frame_info *pfi;
	obj_cnt_type     pfn;

	rc = hal_kvaddr_to_pfn(addrp, &pfn);
	if ( rc != 0 )
		return rc;

	rc = pfn_to_page_frame_info(pfn, &pfi);
	if ( rc != 0 )
		return rc;

	*pp = &pfi->array[ pfn - pfi->min_pfn ];
	
	return 0;
}

/** 指定されたメモリページのマップカウントをインクリメントする
    @param[in] addrp カーネルストレートマップ域内でのアドレス
 */
void
inc_page_map_count(void *addrp) {
	int               rc;
	page_frame        *p;
	intrflags      flags;

	rc = kvaddr_to_page_frame(addrp, &p);
	kassert( rc == 0 );

	spinlock_lock_disable_intr(&p->lock, &flags);
	++p->mapcnt;
	spinlock_unlock_restore_intr(&p->lock, &flags);
}

/** 指定されたメモリページのマップカウントをデクリメントする
    @param[in] addrp カーネルストレートマップ域内でのアドレス
 */
void
dec_page_map_count(void *addrp) {
	int           rc;
	page_frame    *p;
	intrflags  flags;

	rc = kvaddr_to_page_frame(addrp, &p);
	kassert( rc == 0 );

	spinlock_lock_disable_intr(&p->lock, &flags);
	--p->mapcnt;
	if ( p->mapcnt == 0 ) {

		p->state &= ~PAGE_CSTATE_USED;
	}
	spinlock_unlock_restore_intr(&p->lock, &flags);
}

/** 1ページメモリを獲得する
    @param[in] addrp 獲得したカーネルメモリアドレス格納先
    @retval  0        正常獲得完了
    @retval -ENOMEM   メモリ不足により獲得に失敗
 */
int
get_free_page(void **addrp) {

	return alloc_buddy_pages(addrp, 0, KMALLOC_NORMAL);
}

/** メモリを解放する
    @param[in] addrp 解放するカーネルメモリアドレス格納先
    @retval  0        正常解放完了
 */
int
free_page(void *addrp) {
	int           rc;
	page_frame    *p;
	obj_cnt_type pfn;
	intrflags  flags;

	rc = hal_kvaddr_to_pfn(addrp, &pfn);
	kassert( rc == 0 );

	rc = pfn_to_page_frame(pfn, &p);
	kassert( rc == 0 );

	spinlock_lock_disable_intr( &p->buddyp->lock, &flags);

	page_buddy_enqueue(p->buddyp, pfn);

	spinlock_unlock_restore_intr(&p->buddyp->lock, &flags);

	return 0;
}

/** バディープールからメモリを獲得する
    @param[in] addrp   獲得したカーネルメモリアドレス格納先
    @param[in] order   ページオーダ
    @param[in] pgflags ページ獲得条件
    @retval  0        正常獲得完了
    @retval -ENOMEM   メモリ不足により獲得に失敗
 */
int
alloc_buddy_pages(void **addrp, page_order order, 
    pgalloc_flags  __attribute__ ((unused)) pgflags) {
	int               rc;
	void          *kaddr;
	list             *li;
	obj_cnt_type     pfn;
	page_frame_info *pfi;
	intrflags      flags;

	spinlock_lock_disable_intr(&global_pfque.lock, &flags);
	for( li = queue_ref_top( &global_pfque.que );
	     li != (list *)&global_pfque.que;
	     li = li->next ) {

		pfi = CONTAINER_OF(li, page_frame_info, link);
		spinlock_lock(&pfi->buddy.lock);
		
		rc = page_buddy_dequeue(&pfi->buddy, order, &pfn);
		if ( rc == 0 ) {

			rc = hal_pfn_to_kvaddr(pfn, &kaddr);
			kassert( rc == 0 );

			memset(kaddr, 0, PAGE_SIZE);
			*addrp = kaddr;

			rc = 0;

			spinlock_unlock(&pfi->buddy.lock);
			goto unlock_out;
		}

		spinlock_unlock(&pfi->buddy.lock);
	}

unlock_out:
	spinlock_unlock_restore_intr(&global_pfque.lock, &flags);

	return rc;
}

/** バディープールにメモリを返却する
    @param[in] addr    返却するメモリのカーネルメモリアドレス
 */
void
free_buddy_pages(void *addr){

	free_page(addr);
}
