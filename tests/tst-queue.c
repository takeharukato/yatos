/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Test program for queue operation routines                         */
/*                                                                    */
/**********************************************************************/

#include <stddef.h>
#include <stdint.h>

#include <kern/config.h>
#include <kern/assert.h>
#include <kern/string.h>
#include <kern/kprintf.h>
#include <kern/errno.h>
#include <kern/queue.h>
#include <kern/page.h>

#include <kern/tst-progs.h>

#define TEST_DATA (100)  /**< テスト用データの数  */

/** テスト用データ
 */
typedef struct _tst_queue_data{
	list    ent;
	int      no;
}tst_queue_data;

/** テスト用データを格納するキューヘッド
 */
typedef struct _data_head{
	queue   que;
}data_head;

/** テストデータキュー
 */
static data_head qh = {.que = __QUEUE_INITIALIZER(&qh.que), };

/**  データをnoメンバの値をキーに比較する (内部関数)
     @param[in] p1 比較要素1
     @param[in] p2 比較要素2
     @return    正 p1のnoの方が大きい
     @return    負 p2のnoの方が大きい
     @return     0 両者のnoが等しい
 */
static int 
tst_queue_cmp(struct _tst_queue_data *p1, struct _tst_queue_data *p2) {

	if ( p1->no < p2->no )
		return -1;

	if ( p1->no > p2->no )
		return 1;

	return 0;
}

/** キューに対する総データアクセス, キュー内のデータ検索テスト
 */
static void
queue_test2(void) {
	int    i;
	tst_queue_data *d, *found;
	tst_queue_data key;
	list  *lp;
	list *tmp;
	int   cnt;

	/*
	 * キューの初期化
	 */
	queue_init(&qh.que);

	/*
	 * キュー内にデータを追加
	 */
	for( i = 0; TEST_DATA > i; ++i) {

		d = kmalloc(sizeof(tst_queue_data), KMALLOC_NORMAL);
		d->no = i;
		queue_add_top(&qh.que, &d->ent);
	}

	/*
	 * 各キュー要素への順次アクセス
	 */
	cnt = 0;
	queue_for_each(lp, &qh.que) {

		d = CONTAINER_OF(lp, struct _tst_queue_data, ent);
		++cnt;
	}	
	kprintf(KERN_INF, "Queue Test2-1: queue for each count=%d\n", cnt);
	kassert( cnt == TEST_DATA );

	/*
	 * 各キュー要素への逆順アクセス
	 */
	cnt = 0;
	queue_reverse_for_each(lp, &qh.que) {

		d = CONTAINER_OF(lp, struct _tst_queue_data, ent);
		++cnt;
	}	
	kprintf(KERN_INF, "Queue Test2-2: reverse queue for each count=%d\n", cnt);
	kassert( cnt == TEST_DATA );

	/*
	 * 各キュー要素の削除
	 */
	cnt = 0;
	queue_for_each_safe(lp, &qh.que, tmp) {

		d = CONTAINER_OF(lp, struct _tst_queue_data, ent);
		list_del(&d->ent);
		kfree(d);
		++cnt;
	}
	kprintf(KERN_INF, "Queue Test2-3: remove queue entries =%d\n", cnt);
	kassert( cnt == TEST_DATA );

	/*
	 * キュー内にデータを追加
	 */
	for( i = 0; TEST_DATA > i; ++i) {

		d = kmalloc(sizeof(tst_queue_data), KMALLOC_NORMAL);
		d->no = i;
		queue_add_top(&qh.que, &d->ent);
	}
	memset(&key, 0, sizeof(tst_queue_data));
	key.no = TEST_DATA / 2;
	queue_find_element(&qh.que, struct _tst_queue_data, ent, &key, tst_queue_cmp, &found);
	kassert( found != NULL );
	kprintf(KERN_INF, "Queue Test2-4: find data no=%d\n", found->no);

	memset(&key, 0, sizeof(tst_queue_data));
	key.no = TEST_DATA / 4;
	queue_reverse_find_element(&qh.que, struct _tst_queue_data, 
	    ent, &key, tst_queue_cmp, &found);
	kassert( found != NULL );
	kprintf(KERN_INF, "Queue Test2-5: reverse find data no=%d\n", found->no);
	/*
	 * キューの消去
	 */
	cnt = 0;
	queue_reverse_for_each_safe(lp, &qh.que, tmp) {

		d = CONTAINER_OF(lp, struct _tst_queue_data, ent);
		list_del(&d->ent);
		kfree(d);
		++cnt;
	}
	kprintf(KERN_INF, "Queue Test2-6: remove queue entries with reverse order"
	    " count=%d\n", cnt);
	kassert( cnt == TEST_DATA );
}

/** キューの初期化, 追加, 削除テスト
 */
static void
queue_test1(void) {
	int     i;
	tst_queue_data   *d;
	int   cnt;
	list  *lp;
	list *tmp;
	
	queue_init(&qh.que);

	/*
	 * 末尾追加
	 */
	for( i = 0; TEST_DATA > i; ++i) {

		d = kmalloc(sizeof(tst_queue_data), KMALLOC_NORMAL);
		d->no = i;
		queue_add(&qh.que, &d->ent);
	}
	kprintf(KERN_INF, "Queue Test1-1: queue add tail %d times\n", TEST_DATA);

	for( i = 0; TEST_DATA * 2 > i; ++i) {

		lp = queue_get_top(&qh.que);
		queue_add(&qh.que, lp);
	}
	kprintf(KERN_INF, "Queue Test1-2: rotate queue %d times\n", TEST_DATA*2);

	/*
	 * キューを先頭から消去
	 */
	cnt = 0;
	queue_for_each_safe(lp, &qh.que, tmp) {

		d = CONTAINER_OF(lp, struct _tst_queue_data, ent);
		list_del(&d->ent);
		kfree(d);
		++cnt;
	}
	kprintf(KERN_INF, "Queue Test1-3: each for safe queue count=%d\n", cnt);

	/*
	 * 先頭から追加
	 */
	for( i = 0; TEST_DATA > i; ++i) {

		d = kmalloc(sizeof(tst_queue_data), KMALLOC_NORMAL);
		d->no = i;
		queue_add_top(&qh.que, &d->ent);
	}
	kprintf(KERN_INF, "Queue Test1-4: queue add top %d times\n", TEST_DATA);

	/*
	 * キューの逆回転 ( queue_get_last, queue_add_top )
	 */
	for( i = 0; TEST_DATA * 2 > i; ++i) {

		lp = queue_get_last(&qh.que);
		queue_add_top(&qh.que, lp);
	}
	kprintf(KERN_INF, "Queue Test1-5: reverse rotate queue %d times\n", TEST_DATA*2);

	kassert( !queue_is_empty(&qh.que) );
	kprintf(KERN_INF, "Queue Test1-6: NOT empty check ok\n");

	/*
	 * キューを先頭から消去
	 */
	cnt = 0;
	queue_for_each_safe(lp, &qh.que, tmp) {

		d = CONTAINER_OF(lp, struct _tst_queue_data, ent);
		list_del(&d->ent);
		kfree(d);
		++cnt;
	}
	kprintf(KERN_INF, "Queue Test1-7: remove all queue entries count=%d\n", cnt);

	kassert( queue_is_empty(&qh.que) );
	kprintf(KERN_INF, "Queue Test1-8: empty check ok\n");
}

/** キュー構造操作のテスト
 */
void
queue_test(void){

	queue_test1();
	queue_test2();
}
