/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  ID bitmap relevant definitions                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_ID_BITMAP_H)
#define  _KERN_ID_BITMAP_H 

#include <stdint.h>
#include <stddef.h>

#include <kern/kern_types.h>
#include <kern/spinlock.h>
#include <kern/refcount.h>

#define ID_BITMAP_USER                (0)    /**< ユーザ資源を取得              */
#define ID_BITMAP_SYSTEM              (1)    /**< システム資源を取得            */
#define ID_BITMAP_INVALID_ID_MASK     (0x1)  /**< ID 0を予約（不正ID)とする     */
#define ID_BITMAP_FIRST_VALID_ID      (1)    /**< 最初の有効ID                  */
#define ID_BITMAP_IDS_PER_ENT         (64)   /**< 配列1エントリ当たりのID数     */
#define ID_BITMAP_DEFAULT_RESV_IDS    (255)  /**< デフォルトのシステム予約ID数  */
#define ID_BITMAP_DEFAULT_MAP_SIZE    (1024) /**< デフォルトのID数              */

/** IDビットマップ
 */
typedef struct _id_bitmap{
	spinlock                     lock;  /**< IDビットマップ排他用ロック  */
	refcnt                  ref_count;  /**< 参照カウンタ                */
	obj_id               reserved_ids;  /**< システム予約ID数            */
	obj_id                     nr_ids;  /**< 格納可能ID数                */
	uint64_t            max_array_idx;  /**< 配列のインデックス数        */
	uint64_t                     *map;  /**< IDビットマップ              */
}id_bitmap;

/** ID bitmap初期化子
 */
#define __ID_BITMAP_INITIALIZER(nr_rsv)				\
	{							\
	.lock=__SPINLOCK_INITIALIZER,			        \
	.ref_count = __REFCNT_INITIALIZER,	                \
	.reserved_ids = (nr_rsv),		                \
	.nr_ids = 0,                                            \
	.max_array_idx = 0,                                     \
	.map = NULL,				                \
	}

int idbmap_get_specified_id(id_bitmap *_idmap, obj_id _id, int _flags);
int idbmap_get_id(id_bitmap *_idmap, int _idflags, obj_id *_idp);
void idbmap_put_id(id_bitmap *_idmap, obj_id _id);
void idbmap_init(id_bitmap *_idmap, obj_id _reserved_ids);
int idbmap_free(id_bitmap *_idmap);
int idbmap_resize(id_bitmap *_idmap, obj_id _new_ids);
int idbmap_create(obj_id _reserved_ids, id_bitmap **_idmapp);
void idbmap_destroy(id_bitmap *_idmap);
#endif  /*  _KERN_ID_BITMAP_H   */
