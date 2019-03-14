/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  ID bitmap routines                                                */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/spinlock.h>
#include <kern/assert.h>
#include <kern/string.h>
#include <kern/id-bitmap.h>
#include <kern/errno.h>
#include <kern/page.h>


/** IDビットマップ中の不正IDを予約済みにする
    @param[in] idmap      IDビットマップ
 */
static void
reserve_invalid_ids(id_bitmap *idmap) {

	if ( idmap->map != NULL )
		idmap->map[0] |= ID_BITMAP_INVALID_ID_MASK;
}

/** IDビットマップ中の不正IDの予約を解除する
    @param[in] idmap      IDビットマップ
 */
static void
release_invalid_ids(id_bitmap *idmap) {

	if ( idmap->map != NULL )
		idmap->map[0] &= ~ID_BITMAP_INVALID_ID_MASK;
}

/** IDビットマップの大きさを変更する (内部関数)
    @param[in] idmap         IDビットマップ
    @param[in] reserved_ids  予約ID数(単位: ID数)
 */
static void
init_idbmap_common(id_bitmap *idmap, obj_id reserved_ids){

	/*
	 * 各メンバを初期化する
	 */
	spinlock_init( &idmap->lock );
	refcnt_init( &idmap->ref_count );
	idmap->reserved_ids = reserved_ids;
	idmap->nr_ids = 0;
	idmap->max_array_idx = 0;
	idmap->map = NULL;
}

/** IDビットマップの大きさを変更する
    @param[in] idmap      IDビットマップ
    @param[in] new_ids    新しいサイズ(単位: ID数)
    @retval  0        サイズを更新できた
    @retval -EBUSY    伸縮時の削除対象領域に使用中のIDがある
    @retval -EINVAL   新しいサイズに0を指定した. または, 予約ID数を下回るサイズを指定した
 */
static int
resize_bitmap(id_bitmap *idmap, obj_id new_ids){
	int              rc;
	uint64_t          i;
	uint64_t    new_idx;
	uint64_t   *new_map;
	intrflags    iflags;

	if ( new_ids == 0 )
		return -EINVAL;

	if ( new_ids <= idmap->reserved_ids )
		return -EINVAL;

	spinlock_lock_disable_intr(&idmap->lock, &iflags);

	/*  拡張後の配列のインデックス数を算出 */	
	new_idx = ( new_ids + ( ID_BITMAP_IDS_PER_ENT - 1 ) ) / ID_BITMAP_IDS_PER_ENT;

	/*  新しいビットマップを獲得  */
	new_map = kmalloc( sizeof(uint64_t) * new_idx, KMALLOC_NORMAL);
	if ( new_map == NULL ) {

		rc = -ENOMEM;
		goto unlock_out;
	}
	memset(&new_map[0], 0, sizeof(uint64_t) * new_idx);

	/*
	 * 再利用するビットマップをコピーする
	 */
	if ( idmap->max_array_idx < new_idx ) {  /*  拡張する場合  */
		
		/*  既存のマップをコピーする  */
		if ( ( idmap->map != NULL ) && ( idmap->max_array_idx > 0 ) )
			memmove(&new_map[0], &idmap->map[0], 
			    sizeof(uint64_t) * idmap->max_array_idx);
	} else if ( idmap->max_array_idx > new_idx ) {  /*  伸縮する場合  */

		for( i = new_idx; idmap->max_array_idx > i; ++i) {

			if ( idmap->map[i] != 0 ) {  /*  使用中のIDがある  */

				rc = -EBUSY;
				goto free_newmap_out;
			}
		}

		if ( idmap->map != NULL ) /*  既存のマップをコピーする  */
			memmove(&new_map[0], &idmap->map[0], sizeof(uint64_t) * new_idx);
	}

	/*
	 * ビットマップ情報を更新する
	 */
	if ( idmap->map != NULL ) /*  既存のマップを解放する  */
		kfree( idmap->map );

	idmap->map = new_map;
	idmap->max_array_idx = new_idx;
	idmap->nr_ids = new_ids;
	reserve_invalid_ids(idmap);  /*  不正IDが割り当てられないようにする  */

	spinlock_unlock_restore_intr(&idmap->lock, &iflags);

	return 0;

free_newmap_out:
	kfree( new_map );

unlock_out:
	spinlock_unlock_restore_intr(&idmap->lock, &iflags);
	return rc;
}

/** 空いているIDを取得する
    @param[in]     idmap IDビットマップ
    @param[in]     idflags 取得するID種別
    @param[in,out] idp   取得したIDの返却先
    @retval  0      ID取得に成功
    @retval -ENOENT ID取得に失敗
 */
static int
find_free_id(id_bitmap *idmap, int idflags, obj_id *idp) {
	int            rc;
	obj_id         id;
	obj_id     min_id;
	obj_id     max_id;
	uint64_t      idx;
	uint64_t      pos;
	intrflags  iflags;
	
	/*
	 * IDの検索範囲を算出する
	 */
	if ( idflags & ID_BITMAP_SYSTEM ) {  /*  システム用のIDを取得する  */

		min_id = ID_BITMAP_FIRST_VALID_ID;
		max_id = idmap->reserved_ids;
	} else {

		min_id = idmap->reserved_ids;
		max_id = idmap->nr_ids;
	}

	if ( max_id > idmap->nr_ids )
		return -ENOENT;  /*  十分な大きさのマップが割り当てられていない  */

	/*
	 * 空きIDを探す
	 */
	spinlock_lock_disable_intr(&idmap->lock, &iflags);
	for(id = min_id; max_id > id; ++id) {

		/*
		 * 配列のインデクス, エントリ内のビット位置を計算する
		 */
		idx = id / (sizeof(uint64_t) * 8);
		pos = id % (sizeof(uint64_t) * 8);

		if ( idmap->map[idx] & (1ULL << pos) ) 
			continue;  /*  使用中  */
		else {
			/*  空きIDが見つかった  */
			rc = 0;
			*idp = id;
			idmap->map[idx] |= (1ULL << pos);
			goto unlock_out;
		}
	}
	
	rc = -ENOENT;  /*  空きIDがない  */

unlock_out:
	spinlock_unlock_restore_intr(&idmap->lock, &iflags);

	return rc;
}

/** IDビットマップのビットマップを解放する(実処理関数)
    @param[in] idmap        IDビットマップ
    @retval  0     正常終了
    @retval -EBUSY 使用中のIDがある
    @note   静的に確保したビットマップ用
 */
static int
free_map_in_idmap(id_bitmap *idmap) {
	int            rc;
	uint64_t        i;
	intrflags  iflags;

	spinlock_lock_disable_intr(&idmap->lock, &iflags);
	
	release_invalid_ids(idmap);  /*  不正IDの予約を解除する  */

	for(i = 0; idmap->max_array_idx > i; ++i) {

		if ( idmap->map[i] != 0 ) { /*   使用中のIDがある  */
			
			reserve_invalid_ids(idmap); /*  不正IDを予約済みに戻す  */
			rc = -EBUSY;
			goto unlock_out;
		}
	}

	/*
	 * 格納ID数/最大インデックスを初期化
	 */
	idmap->nr_ids = 0;
	idmap->max_array_idx = 0;

	kfree( idmap->map );  /*  ビットマップを解放する  */
	idmap->map = NULL;

	spinlock_unlock_restore_intr(&idmap->lock, &iflags);

	return 0;

unlock_out:	
	spinlock_unlock_restore_intr(&idmap->lock, &iflags);

	return rc;
}

/** ID bitmapへの参照を得る
    @param[in] idmap    IDビットマップ
    @retval  0      正常終了
    @retval -ENOENT 削除中
 */
static int
get_idbmap_ref(id_bitmap *idmap) {
	int            rc;
	intrflags  iflags;

	spinlock_lock_disable_intr(&idmap->lock, &iflags);

	rc = refcnt_get( &idmap->ref_count, NULL );
	if ( rc != 0 ) {
		
		rc = -ENOENT;
		goto unlock_out;
	}

	rc = 0;

unlock_out:
	spinlock_unlock_restore_intr(&idmap->lock, &iflags);

	return rc;
}

/** ID bitmapへの参照を解放する
    @param[in] idmap IDビットマップ
    @return 加算前の参照カウンタ値
 */
static void
put_idbmap_ref(id_bitmap *idmap) {
	int               rc;
	refcnt_val    oldcnt;
	intrflags     iflags;

	spinlock_lock_disable_intr(&idmap->lock, &iflags);
	rc = refcnt_put( &idmap->ref_count, &oldcnt );
	if ( rc == 0 ) {

		kassert( oldcnt == 1 );
		spinlock_unlock_restore_intr(&idmap->lock, &iflags);
		rc = free_map_in_idmap(idmap);   /* IDマップを解放 */
		kassert( rc == 0 );
		
		kfree( idmap );  /*  IDビットマップを解放する  */

		return ;
	}
	spinlock_unlock_restore_intr(&idmap->lock, &iflags);

	return ;
}

/** 指定したIDを取得する
    @param[in] idmap   IDビットマップ
    @param[in] id      取得するID
    @param[in] idflags 取得するID種別
    @retval  0        IDを取得できた。
    @retval -ENOENT   削除中のIDビットマップを獲得しようとした
    @retval -EBUSY    IDがすでに使用されている
    @retval -EINVAL   システムIDを取得しようとしたが, 指定されたIDがユーザIDの範囲にある
                     不正なIDを指定した
 */
int
idbmap_get_specified_id(id_bitmap *idmap, obj_id id, int idflags){
	int            rc;
	uint64_t      idx;
	uint64_t      pos;
	intrflags  iflags;

	if ( ( idflags & ID_BITMAP_SYSTEM ) && ( id >= idmap->reserved_ids ) )
		return -EINVAL;

	rc = get_idbmap_ref(idmap);  /*  ID獲得に伴い参照カウンタを上げる  */
	if ( rc != 0 )
		return -ENOENT;

	/*
	 * 指定されたIDの配列インデクス, ビット位置を算出
	 */
	idx = id / ( sizeof(uint64_t) * 8 );
	pos = id % ( sizeof(uint64_t) * 8 );

	if ( idx >= idmap->max_array_idx ) {  /*  ビットマップを拡張する  */

		rc = resize_bitmap(idmap,
		    idmap->nr_ids + ID_BITMAP_DEFAULT_MAP_SIZE);
		if ( rc != 0 )
			goto error_out;
	}

	/*
	 * 空きIDを得る
	 */
	spinlock_lock_disable_intr(&idmap->lock, &iflags);
	if ( idmap->map[idx] & (1ULL << pos) ) {  /*  IDが既に使用されている  */

		rc = -EBUSY;
		goto unlock_out;
	}

	idmap->map[idx] |= (1ULL << pos);  /*  IDを使用中にする  */

	spinlock_unlock_restore_intr(&idmap->lock, &iflags);

	return 0;

unlock_out:
	spinlock_unlock_restore_intr(&idmap->lock, &iflags);

error_out:
	put_idbmap_ref(idmap);  /*  ID獲得失敗に伴い参照カウンタを下げる  */
	return rc;
}

/** 空いているIDを取得する
    @param[in]     idmap IDビットマップ
    @param[in]     idflags 取得するID種別
    @param[in,out] idp   取得したIDの返却先
    @retval   0      ID取得に成功
    @retval  -ENOENT ID取得に失敗したか, または, 削除中のIDビットマップを獲得しようとした
    @retval  -ENOMEM メモリ不足
 */
int
idbmap_get_id(id_bitmap *idmap, int idflags, obj_id *idp) {
	int find_id_rc;
	int         rc;
	obj_id   newid;

	rc = get_idbmap_ref(idmap);  /*  ID獲得に伴い参照カウンタを上げる  */
	if ( rc != 0 )
		return -ENOENT;

	do{
		/*  空きIDを検索する  */
		find_id_rc = find_free_id(idmap, idflags, &newid);
		if ( find_id_rc == -ENOENT ) {

			/* ビットマップを拡張して空きIDを得る  */
			rc = resize_bitmap(idmap,
			    idmap->nr_ids + ID_BITMAP_DEFAULT_MAP_SIZE);
			if ( rc != 0 )
				goto error_out;
			
			continue;
		}
	} while ( find_id_rc != 0 );

	*idp = newid;

	return 0;

error_out:
	put_idbmap_ref(idmap);  /*  ID獲得失敗に伴い参照カウンタを下げる  */
	return rc;
}

/** IDを返却する
    @param[in] idmap IDビットマップ
    @param[in] id    返却するID
 */
void
idbmap_put_id(id_bitmap *idmap, obj_id id) {
	uint64_t      idx;
	uint64_t      pos;
	intrflags  iflags;

	/*
	 * 配列のインデクス, エントリ内のビット位置を計算する
	 */
	idx = id / (sizeof(uint64_t) * 8);
	pos = id % (sizeof(uint64_t) * 8);

	spinlock_lock_disable_intr(&idmap->lock, &iflags);

	kassert( ( idmap->map[idx] & (1ULL << pos) ) );  /*  使用中でなければならない  */

	idmap->map[idx] &= ~(1ULL << pos);  /*  IDを解放する  */

	spinlock_unlock_restore_intr(&idmap->lock, &iflags);

	put_idbmap_ref(idmap);  /*  ID解放に伴い参照カウンタを下げる  */

	return;
}

/** IDビットマップのサイズを更新する
    @param[in] idmap        IDビットマップ
    @param[in] new_ids      更新後の登録可能ID数
    @retval  0       正常終了
    @retval -ENOENT 削除中のIDビットマップを獲得しようとした
    @retval -ENOMEM  メモリ不足
    @retval -EINVAL  登録可能IDが予約IDの範囲より小さい
 */
int
idbmap_resize(id_bitmap *idmap, obj_id new_ids) {
	int rc;

	rc = get_idbmap_ref(idmap);  /*  参照カウンタを上げる  */
	if ( rc != 0 )
		return -ENOENT;

	rc = resize_bitmap(idmap, new_ids);

	put_idbmap_ref(idmap);  /*  参照カウンタを下げる  */

	return rc;
}

/** IDビットマップを初期化する
    @param[in] idmap        IDビットマップ
    @param[in] reserved_ids 予約ID数
    @note   __ID_BITMAP_INITIALIZER相当の機能を提供するIF
 */
void
idbmap_init(id_bitmap *idmap, obj_id reserved_ids) {

	init_idbmap_common(idmap, reserved_ids);	
}

/** IDビットマップのビットマップを解放する
    @param[in] idmap        IDビットマップ
    @retval  0     正常終了
    @retval -EINVAL 動的確保したIDビットマップをfreeしようとした
    @retval -ENOENT 削除中のIDビットマップを獲得しようとした
    @retval -EBUSY 使用中のIDがある
    @note   静的に確保したビットマップ用
 */
int
idbmap_free(id_bitmap *idmap) {
	int            rc;

	rc = get_idbmap_ref(idmap);  /*  参照カウンタを上げる  */
	/*  動的確保したIDビットマップしか削除予約中にならないため,
	 *  参照カウンタの獲得に失敗すると言うことは, idbmap_free
	 *  の対象外となるビットマップを解放しようとしたことになる
	 */
	if ( rc != 0 )
		return -EINVAL;  

	rc = free_map_in_idmap(idmap);   /* IDマップを解放 */
	if ( rc != 0 ) 
		goto error_out;

	put_idbmap_ref(idmap);  /*  参照カウンタを下げる  */

	return 0;

error_out:
	put_idbmap_ref(idmap);  /*  参照カウンタを下げる  */
	return rc;
}

/** IDビットマップの解放を予約する
    @param[in] idmap IDビットマップ
 */
void
idbmap_destroy(id_bitmap *idmap) {
	intrflags  iflags;

	spinlock_lock_disable_intr(&idmap->lock, &iflags);
	refcnt_mark_deleted( &idmap->ref_count );   /*  削除予約  */
	spinlock_unlock_restore_intr(&idmap->lock, &iflags);

	put_idbmap_ref(idmap);  /*  init_idbmap_commonで獲得した参照カウンタを解放する  */

	return ;
}

/** IDビットマップを生成する
    @param[in] reserved_ids 予約IDの範囲
    @param[out] idmapp      IDビットマップを指し示すポインタのアドレス
    @retval  0      正常終了
    @retval -ENOMEM メモリ不足
 */
int
idbmap_create(obj_id reserved_ids, id_bitmap **idmapp){
	int           rc;
	id_bitmap *idmap;

	idmap = kmalloc( sizeof(id_bitmap), KMALLOC_NORMAL);
	if ( idmap == NULL ) {

		rc = -ENOMEM;
		goto error_out;
	}

	/*
	 * 各メンバを初期化する
	 */
	init_idbmap_common(idmap, reserved_ids); /*  参照カウンタを1に設定する  */

	*idmapp = idmap;

	return 0;

error_out:
	return rc;
}

