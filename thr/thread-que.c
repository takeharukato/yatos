/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Thread queue operation routines                                   */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/errno.h>
#include <kern/spinlock.h>
#include <kern/list.h>
#include <kern/queue.h>
#include <kern/thread.h>

/** スレッドキューにスレッドを追加する
    @param[in] q   追加先のスレッドキュー
    @param[in] thr 追加するスレッド
    @retval 追加後のスレッド格納数
    @note キュー q のロックを呼出元で獲得して呼び出すこと
 */
int
tq_add(thread_queue *q, struct _thread *thr) {
	
	kassert( q != NULL);
	kassert( thr != NULL);
	kassert( spinlock_locked_by_self( &q->lock ) );

	queue_add( &q->que, &thr->link );  /*  キューにスレッドを追加する                */
	++q->cnt;                               /*  キュー中のスレッド数をインクリメントする  */

	return q->cnt;  /*  追加後のスレッド数を返却する  */
}

/** スレッドキューからスレッドを取り外す
    @param[in] q   操作対象のスレッドキュー
    @param[in] thr 取り除くするスレッド
    @note キュー q のロックを呼出元で獲得して呼び出すこと
 */
int
tq_del(thread_queue *q, struct _thread *thr) {

	kassert( q != NULL);
	kassert( thr != NULL);
	kassert( spinlock_locked_by_self( &q->lock ) );

	list_del( &thr->link );  /* スレッドをキューから取り除く  */
	--q->cnt;            /*  取り出し後のキュー内のスレッド数を減算する  */

	return q->cnt;  /*  取り外した後のスレッド数を返却する  */
}

/** スレッドキューの先頭のスレッドを取り出す
    @param[in] q      操作対象キュー
    @param[in] thrp   キューの先頭スレッドを設定するポインタ変数のアドレス
    @retval 取り出し後のスレッド格納数
    @note キュー q のロックを呼出元で獲得しておき, キューが
    空でないことを呼出元責任で保証すること
 */
int
tq_get_top(thread_queue *q, struct _thread **thrp) {

	kassert( q != NULL);
	kassert( spinlock_locked_by_self( &q->lock ) );
	kassert( !queue_is_empty( &q->que ) );

	*thrp = CONTAINER_OF( queue_get_top( &q->que ), 
	    thread, link);   /*  キューからスレッドを取り出す                */
	--q->cnt;            /*  取り出し後のキュー内のスレッド数を減算する  */

	return q->cnt;       /*  取り出し後のキュー内のスレッド数を返却する  */
}

/** スレッドキューが空であることを確認する
    @param[in] q 確認対象キュー
    @note キュー q のロックを呼出元で獲得して呼び出すこと
 */
bool
tq_is_empty(thread_queue *q) {

	kassert( q != NULL);
	kassert( spinlock_locked_by_self(&q->lock) );

	return queue_is_empty( &q->que );  /*  キューが空である場合は真を返す  */
}

/** スレッドキューを初期化する
    @param[in] q 初期化対象キュー
 */
void
tq_init(thread_queue *q) {

	kassert( q != NULL);

	spinlock_init( &q->lock );   /*  スピンロックを初期化する          */
	queue_init( &q->que );  /* キューを初期化する                 */
	q->cnt = 0;                  /* キュー内のスレッド数を0に設定する  */
}
