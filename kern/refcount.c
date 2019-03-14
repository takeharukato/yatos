/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Reference counter routines                                        */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/spinlock.h>
#include <kern/refcount.h>
#include <kern/errno.h>

/**
   参照カウンタの初期化
    @param[in] counterp 操作対象の参照カウンタ
 */
void
refcnt_init(refcnt *counterp){

	/*
	 * ロック, カウンタ, 削除要求を初期化
	 */
	spinlock_init(&counterp->lock);
	counterp->counter = REFCNT_INITIAL_VAL;
	counterp->deleted = false;
}

/**
   参照カウンタの獲得
   @param[in] counterp 操作対象の参照カウンタ
   @param[out] valp    操作前の参照カウンタの値を返却する領域
   @retval  0        正常終了
   @retval -ENOENT   削除要求済みのオブジェクトの参照を得ようとした
 */
int
refcnt_get(refcnt *counterp, refcnt_val *valp){
	int             rc;
	refcnt_val old_cnt;

	/*
	 * 参照カウンタを上げる
	 */
	spinlock_lock(&counterp->lock);

	old_cnt = counterp->counter;  /*  削除要求判定前に以前の値を取得  */
	if ( counterp->deleted ) {

		rc = -ENOENT;
		goto unlock_out;  /*  削除要求中のため獲得失敗  */
	}

	++counterp->counter;

	rc = 0;

unlock_out:
	spinlock_unlock(&counterp->lock);

	if ( valp != NULL )
		*valp = old_cnt;

	return rc;
}

/**
   参照カウンタの解放
   @param[in] counterp 操作対象の参照カウンタ
   @param[out] valp    操作前の参照カウンタの値を返却する領域
   @retval     0       削除可能
   @retval    -EBUSY   オブジェクトへの参照者が残存
 */
int
refcnt_put(refcnt *counterp, refcnt_val *valp){
	int             rc;
	refcnt_val old_cnt;

	/*
	 * 参照カウンタを下げる
	 */
	spinlock_lock(&counterp->lock);
	kassert( counterp->counter > 0 );
 
	old_cnt = counterp->counter;
	--counterp->counter;

	rc = -EBUSY;
	if ( ( old_cnt == REFCNT_INITIAL_VAL ) && ( counterp->deleted ) ) 
		rc = 0;  /*  削除可能  */

	spinlock_unlock(&counterp->lock);
	
	if ( valp != NULL )
		*valp = old_cnt;

	return rc;
}

/**
   参照カウンタ付きオブジェクト削除を要求する
   @param[in] counterp 操作対象の参照カウンタ
   @retval     0       削除可能
 */
void
refcnt_mark_deleted(refcnt *counterp){

	/*
	 * 参照カウンタの削除要求をセットする
	 */
	spinlock_lock(&counterp->lock);
	counterp->deleted = true;	
	spinlock_unlock(&counterp->lock);
}
