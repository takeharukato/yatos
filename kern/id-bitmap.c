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
#include <kern/spinlock.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/string.h>
#include <kern/kprintf.h>
#include <kern/id-bitmap.h>
#include <kern/errno.h>
#include <kern/kresv-ids.h>

/** 指定したIDを返却する
    @param[in] idmap IDビットマップ
    @param[in] id    取得するID
    @param[in]       idflags 取得するID種別
    @retval  0       IDを取得できた。
    @retval -EBUSY   IDがすでに使用されている
    @retval -EINVAL  システムIDを取得しようとしたが, 指定されたIDがユーザIDの範囲にある
 */
int
get_specified_id(id_bitmap *idmap, obj_id id, int idflags) {
	int            rc;
	uint64_t      idx;
	uint64_t      pos;
	intrflags flags;

	if ( id >= MAX_OBJ_ID )
		return 0;

	if ( ( idflags & ID_FLAG_SYSTEM ) && ( id > SYS_RESV_MAX_ID ) )
		return -EINVAL;

	idx = id / (sizeof(uint64_t) * 8);
	pos = id % (sizeof(uint64_t) * 8);

	spinlock_lock_disable_intr(&idmap->lock, &flags);
	if ( idmap->map[idx] & (1ULL << pos) ) {

		rc = -EBUSY;
		goto error_out;
	}

	idmap->map[idx] |= (1ULL << pos);
	rc = 0;

error_out:
	spinlock_unlock_restore_intr(&idmap->lock, &flags);

	return rc;
}

/** 空いているIDを取得する
    @param[in]     idmap IDビットマップ
    @param[in,out] idp   取得したIDの返却先
    @param[in]     idflags 取得するID種別
    @retval  0      ID取得に成功
    @retval -ENOENT ID取得に失敗
 */
int
get_id(id_bitmap *idmap, obj_id *idp, int idflags) {
	int            rc;
	obj_id         id;
	obj_id     min_id;
	obj_id     max_id;
	uint64_t      idx;
	uint64_t      pos;
	intrflags flags;

	if ( idflags & ID_FLAG_SYSTEM ) {

		min_id = 0;
		max_id = SYS_RESV_MAX_ID + 1;
	} else {

		min_id = SYS_RESV_MAX_ID + 1;
		max_id = MAX_OBJ_ID;
	}

	spinlock_lock_disable_intr(&idmap->lock, &flags);
	for(id = min_id; id < max_id; ++id) {


		idx = id / (sizeof(uint64_t) * 8);
		pos = id % (sizeof(uint64_t) * 8);

		if ( idmap->map[idx] & (1ULL << pos) ) {
			continue;
		} else {

			rc = 0;
			*idp = id;
			idmap->map[idx] |= (1ULL << pos);
			goto unlock_out;
		}
	}
	
	rc = -ENOENT;
unlock_out:
	spinlock_unlock_restore_intr(&idmap->lock, &flags);

	return rc;
}


/** IDを返却する
    @param[in] idmap IDビットマップ
    @param[in] id    返却するID
 */
void
put_id(id_bitmap *idmap, obj_id id) {
	uint64_t      idx;
	uint64_t      pos;
	intrflags   flags;

	idx = id / (sizeof(uint64_t) * 8);
	pos = id % (sizeof(uint64_t) * 8);

	spinlock_lock_disable_intr(&idmap->lock, &flags);

	kassert( ( idmap->map[idx] & (1ULL << pos) ) );

	idmap->map[idx] &= ~(1ULL << pos);

	spinlock_unlock_restore_intr(&idmap->lock, &flags);

	return;
}

/** IDが使用中であることを確認する
 */
bool
is_id_busy(id_bitmap *idmap, obj_id id){
	bool           rc;
	uint64_t      idx;
	uint64_t      pos;
	intrflags   flags;

	idx = id / (sizeof(uint64_t) * 8);
	pos = id % (sizeof(uint64_t) * 8);

	spinlock_lock_disable_intr(&idmap->lock, &flags);

	kassert( ( idmap->map[idx] & (1ULL << pos) ) );

	rc = ( idmap->map[idx] & (1ULL << pos) );

	spinlock_unlock_restore_intr(&idmap->lock, &flags);

	return rc;
}
/** IDビットマップを初期化する
    @param[in] idmap IDビットマップ
 */
void
init_id_bitmap(id_bitmap *idmap) {
	int rc;
	int  i;

	spinlock_init(&idmap->lock);
	memset(&idmap->map[0], 0, sizeof(uint64_t) * ID_BITMAP_ARRAY_IDX);

        /*  カーネル内システムスレッドIDを予約する  */
	for( i = 0; ID_NR_RESVED > i; ++i ) {

		rc = get_specified_id(idmap, i, ID_FLAG_SYSTEM);
		kassert( rc == 0 );
	}
}
