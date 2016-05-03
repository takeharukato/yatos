/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  page frame DB buddy page management part                          */
/*                                                                    */
/**********************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/param.h>
#include <kern/kprintf.h>
#include <kern/spinlock.h>
#include <kern/errno.h>
#include <kern/assert.h>
#include <kern/page.h>
#include <kern/page-buddy.h>

//#define ENQUEUE_PAGE_DEBUG
//#define ENQUEUE_PAGE_LOOP_DEBUG
//#define DEQUEUE_PAGE_DEBUG

/**  バディからページを取り出す
     @param[in] buddy   バディページプール管理情報
     @param[in] rm_page バディから取り出した要求オーダ以上で最初に見つかった空きページ
     @param[in] request_order 要求ページオーダ
     @param[in] cur_order rm_pageのページオーダ
     @return 返却する要求ページオーダのページに対するページフレーム情報
     @note 取得したページを1対のバディの組みとみなし, 前半をページキューに追加
           後半を次のオーダのキューに追加する処理を要求されたオーダになるまで
	   繰り返す(最初のページの最後尾から要求オーダ分のページを返却する)
 */
static page_frame *
remove_page_from_page_queue(page_buddy *buddy, page_frame *rm_page, 
    page_order request_order, page_order cur_order){
	size_t              size;
	queue          *pg_queue;

	kassert(rm_page->order == cur_order);   /*  申告されたオーダが正しいことを確認  */

	pg_queue = &buddy->page_list[cur_order];  /*  ページを取り出したキューを参照  */

	size = 1 << cur_order;  /*  ページオーダからページサイズを算出  */

	while (cur_order > request_order) { /* 要求より大きいページを切り出した場合 */
		
		--cur_order;  /*  ページオーダを一つ下げる  */
		pg_queue = &buddy->page_list[cur_order]; /*  操作対象のページキューを更新  */

		size >>= 1;  /*  ページサイズを更新  */

		rm_page->order = cur_order;  /*  取り出したページのページオーダを更新  */
		
		kassert( !PAGE_CSTATE_NOT_FREED(rm_page) );

		queue_add(pg_queue, &rm_page->link);  /*  ページをキューに追加  */
		++buddy->free_nr[cur_order];
#if defined(DEQUEUE_PAGE_DEBUG)
		kprintf(KERN_DBG, "add %p (next:%p, count:%d) to %p size %u "
		    "order:%d[%u] req:%d \n", 
		    rm_page, rm_page + size, 
		    (int)(((void *)(rm_page + size) - (void *)rm_page)/sizeof(page_frame)), 
		    pg_queue, (unsigned int)size, (int)cur_order,
		    (unsigned int)buddy->free_nr[cur_order], (int)request_order);
#endif  /*  KERN_DEQUEUE_PAGE_DEBUG  */

		rm_page += size;  /*  取り出したページのバディ（対向ページ）を参照   */
		rm_page->order = cur_order;   /*  バディ側のページオーダを更新  */
	}

	if ( rm_page->state & PAGE_CSTATE_RESERVED ) {

		/*  予約ページが含まれる場合は, PANIC  */
		kprintf(KERN_CRI, "Reserved page is dequeued in remove from page queue: "
		    "%p pfn:%u flags=%x\n", rm_page, rm_page->pfn, rm_page->state);
		kassert(0);
	}
		
	return rm_page;
}

/** バディプールにページを追加する
    @param[in] buddy 追加対象のバディプール
    @param[in] pfn   追加するページのページフレーム番号
 */
void
page_buddy_enqueue(page_buddy *buddy, obj_cnt_type pfn) {
	int                   rc;
	page_order         order;
	page_order     cur_order;
	obj_cnt_type     cur_idx;
	obj_cnt_type   buddy_idx;
	page_frame     *req_page;
	page_frame         *base;
	page_frame     *cur_page;
	page_frame   *buddy_page;
	queue              *area;
	page_order_mask     mask;
	intrflags          flags;

	kassert( buddy->array != NULL );
	kassert( ( buddy->start_pfn <= pfn ) &&
	    ( pfn < ( buddy->start_pfn + buddy->nr_pages ) ) );
	
	/*  解放対象のページフレーム情報を得る */
	rc = pfn_to_page_frame(pfn, &req_page);
	if ( rc != 0 )  /*  ページフレームは利用可能な物理ページではない  */
		return ;

	cur_order = order = req_page->order;  /*  ページオーダを取得  */
	kassert( order < PAGE_POOL_MAX_ORDER);

	/*  
	 *  バディページのページフレーム番号を取得するマスク
	 */
	mask =  ( ~((page_order_mask)0) ) << order;

	/*  バディの先頭ページを取得  */
	base = &buddy->array[0];

	/*  ページ配列のインデクスを算出  */
	kassert( (uintptr_t)req_page >= (uintptr_t)base );
	cur_idx = req_page - base;

	/*  ページインデクスが2のべき乗であることを確認  */
	kassert( !(cur_idx & ~mask) );

	/*  解放対象のページのキューを取得  */
	area = &buddy->page_list[cur_order];
	
	/*  バディ全体のロックを獲得  */
	spinlock_lock_disable_intr(&buddy->lock, &flags);

	/*  ページを開放状態に設定  */
	req_page->state &= ~PAGE_CSTATE_USED;

	/*  最大オーダまでページの解放処理を繰り返す  */
	while(cur_order < PAGE_POOL_MAX_ORDER ) {
		
		/*  ページキューが対象のバディ内になければならない  */
		kassert( ( (uintptr_t)area ) < 
		    ( (uintptr_t)&buddy->page_list[PAGE_POOL_MAX_ORDER] ) );

		/*  ページインデクスがバディのページ数内に収まらなければならない  */
		kassert(cur_idx < buddy->nr_pages);

		/*  格納対象のページフレーム情報を更新  */
		cur_page = &base[cur_idx];

		/*  格納対象のページオーダを更新  */
		cur_page->order = cur_order;

		/*  配列内に格納不能なオーダだった場合は, 接続不能  */
		if ( cur_order >= (PAGE_POOL_MAX_ORDER - 1) )
			break;

		/*  バディページのインデクスを算出  */
		buddy_idx = cur_idx ^ ( 1 << cur_order );

		/*  バディページのインデクスが配列の要素数を超える場合は接続不能  */
		if (buddy_idx >= buddy->nr_pages) 
			break;

		/*  バディページのページフレーム情報を取得  */
		buddy_page = &base[buddy_idx];

#if  defined(ENQUEUE_PAGE_LOOP_DEBUG)
		kprintf(KERN_DBG, "enque-dbg mask 0x%lx cur_idx %u, buddy_idx %u "
		    "cur:%p buddy:%p array%p[%d]\n", 
		    mask, cur_idx, buddy_idx, cur_page, buddy_page, 
		    area, cur_order);
#endif  /*  ENQUEUE_PAGE_LOOP_DEBUG  */

		/*  
		 *  互いのオーダが異なる場合は接続不能
		 */
		if (cur_page->order != buddy_page->order)
			break;

		/*  いずれかのページが使用中だった場合は接続不能  */
		if ( PAGE_CSTATE_NOT_FREED(buddy_page) )
			break;

		/*  バディページをキューから外す  */
		list_del(&buddy_page->link);
		--buddy->free_nr[buddy_page->order];

		/*  ページオーダーを一段あげる  */
		mask <<= 1;
		++cur_order;

		/*  ページキューを更新する  */
		area = &buddy->page_list[cur_order];

		/*  ページのインデクスを当該オーダでの先頭ページに更新する  */
		cur_idx &= mask;
	}

	/* 追加対象ページのインデクスと追加対象キューがレンジ内にあることを確認 */
	kassert(cur_idx < buddy->nr_pages);
	cur_page = &base[cur_idx];

	kassert( cur_page->order < PAGE_POOL_MAX_ORDER );
	area = &buddy->page_list[cur_page->order];

#if  defined(ENQUEUE_PAGE_DEBUG)
	kprintf(KERN_DBG, "pfn %d addr:%p order=%d add into %p[%d]\n", 
	    cur_idx, (void *)( (uintptr_t)(cur_idx << PAGE_SHIFT) ), 
	    cur_page->order, area, (buddy->free_nr[cur_page->order] + 1));
#endif  /*  ENQUEUE_PAGE_DEBUG  */

	/*
	 * ページをキューに追加する  
	 */
	queue_add(area, &cur_page->link);  
	++buddy->free_nr[cur_page->order];
	spinlock_unlock_restore_intr(&buddy->lock, &flags);

	return;
}

/** バディページから所定のオーダのページを取り出しページフレーム番号を返す
    @param[in] buddy バディページ
    @param[in] order   取得するページのオーダ
    @param[out] pfnp   取得したページのページフレーム番号を返却する領域
    @retval     0      正常にページを獲得した
    @retval    -EINVAL ページフレーム情報返却域が不正か要求したページオーダが不正
    @retval    -ENOMEM 空きページがない
 */
int
page_buddy_dequeue(page_buddy *buddy, page_order order, obj_cnt_type *pfnp){
	page_order cur_order;
	intrflags      flags;
	page_frame *cur_page;

	if ( (pfnp == NULL) || 
	    (order >= PAGE_POOL_MAX_ORDER) )
		return -EINVAL;  /* ページフレーム情報返却域 or 要求したページオーダが不正 */

	cur_order = order;

	spinlock_lock_disable_intr(&buddy->lock, &flags);

	while (cur_order < PAGE_POOL_MAX_ORDER) { /* 空きページがあるキューを順番に調べる */

		if ( !queue_is_empty(&buddy->page_list[cur_order]) ) {

			cur_page = CONTAINER_OF(
				queue_get_top(&buddy->page_list[cur_order]),
				page_frame, link); /* 空きページを取り出す */
			--buddy->free_nr[cur_order];

                        /* 空きページから要求オーダのページを切り出す */
			cur_page = remove_page_from_page_queue(buddy, 
			    cur_page, order, cur_order);  

			cur_page->state |= PAGE_CSTATE_USED; /* ページを使用中にする */
			spinlock_unlock_restore_intr(&buddy->lock, &flags);

			*pfnp = cur_page->pfn;  /* ページフレーム番号を返却する  */

			if ( cur_page->order != order ) {
				
				/*  ページオーダが一致しない場合は内部整合性異常  */
				kprintf(KERN_CRI, "Invalid order page is allocated:%p "
				    "pfn:%u flags=%x order:%d reqest-order:%d find:%d\n", 
				    cur_page, cur_page->pfn, cur_page->state,
				    cur_page->order, order, cur_order);
				kassert(0);
			}

			kassert( ( buddy->start_pfn <= cur_page->pfn ) &&
			    ( cur_page->pfn < ( buddy->start_pfn + buddy->nr_pages ) ) );

			return 0;
		}

		++cur_order;  /*  より上のオーダからページを切り出す  */
	}

	spinlock_unlock_restore_intr(&buddy->lock, &flags);

	return -ENOMEM;
}

/** フリーページ数を得る
    @param[in] buddy バディ管理情報へのポインタ
    @return バディプール中の空きページ数
 */
obj_cnt_type
page_buddy_get_free(page_buddy *buddy){
	int i;
	obj_cnt_type free_nr;

	for(i = 0, free_nr = 0; i < PAGE_POOL_MAX_ORDER; ++i) {

		free_nr += buddy->free_nr[i] * ( 1 << i );
	}

	return free_nr;
}

/** バディプールを初期化する
    @param[in] buddy バディー管理情報へのポインタ
    @param[in] array ページフレーム情報配列のアドレス
    @param[in] start_pfn ページフレーム情報配列の先頭項目のページフレーム番号
    @param[in] nr_pfn buddyで管理しているページの数
 */
void
page_buddy_init(page_buddy *buddy, page_frame *array, obj_cnt_type start_pfn,
    obj_cnt_type nr_pfn) {
	int i;

	/* buddyのページキューを初期化 */
	for(i = 0; i < PAGE_POOL_MAX_ORDER; ++i) {
		
		buddy->free_nr[i] = 0;
		queue_init(&buddy->page_list[i]);
	}

	/* ロック, 管理情報の初期化  */
	spinlock_init(&buddy->lock);
	buddy->array = array;
	buddy->start_pfn = start_pfn;
	buddy->nr_pages = nr_pfn;
}
