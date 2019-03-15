/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Event mask operation routines                                     */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/errno.h>
#include <kern/spinlock.h>
#include <kern/async-event.h>
#include <kern/queue.h>
#include <kern/list.h>

/** イベントマップをクリアする（実装部)
    @param[in] map 操作対象のイベントマップ
 */
static void 
event_map_clr_impl(events_map *map) {
	int i;

	for( i = 0; EV_MAP_LEN > i ; ++i ) 
		map[i] = 0;
}

/** イベントマップを埋める（実装部)
    @param[in] map 操作対象のイベントマップ
 */
static void 
event_map_fill_impl(events_map *map) {
	int   i;
	
	for( i = 0; EV_MAP_LEN > i ; ++i ) 
		map[i] = ~( (events_map)0 );
}

/** イベントマップ中の所定のビットがマスクされていることを確認する（実装部)
    @param[in] map 操作対象のイベントマップ
    @param[in] id  イベントID
    @retval    真  イベントマップ中の所定のビットがマスクされている
    @retval    偽  イベントマップ中の所定のビットがマスクされていない
 */
static bool
ev_mask_test_impl(events_map *map, event_no id) {
	int          pos;
	int          idx;

	pos = id / ( sizeof(events_map) * 8 );
	idx = id % ( sizeof(events_map) * 8 );

	return ( map[pos] & ( ( (events_map)1 ) << idx ) );
}

/** イベントマップが空であることを確認する(実装部)
    @param[in] map 操作対象のイベントマップ
    @retval    真  イベントマップが空である
    @retval    偽  イベントマップが空でない
 */
static bool
ev_mask_empty_impl(events_map *map) {
	int pos;

	for( pos = 0; EV_MAP_LEN > pos; ++pos) {

		if ( map[pos] != 0 )
			return false;
	}

	return true;
}

/** イベントマップ中の所定のビットをマスクする（実装部)
    @param[in] map 操作対象のイベントマップ
    @param[in] id イベントID
 */
static void 
ev_mask_set_impl(events_map *map, event_no id) {
	int          pos;
	int          idx;

	pos = id / ( sizeof(events_map) * 8 );
	idx = id % ( sizeof(events_map) * 8 );

	map[pos] |= ( (events_map)1 ) << idx;
}

/** イベントマップ中の所定のビットをクリアする（実装部)
    @param[in] map 操作対象のイベントマップ
    @param[in] id イベントID
 */
static void 
ev_mask_unset_impl(events_map *map, event_no id) {
	int          pos;
	int          idx;

	pos = id / ( sizeof(events_map) * 8 );
	idx = id % ( sizeof(events_map) * 8 );

	map[pos] &= ~( ( (events_map)1 ) << idx );
}

/** イベントマスクをクリアする
    @param[in] mask 操作対象のマスク
 */
void
ev_mask_clr(event_mask *mask) {

	event_map_clr_impl( &mask->map[0] );
}

/** イベントマスクを埋める
    @param[in] mask 操作対象のマスク
 */
void
ev_mask_fill(event_mask *mask) {

	event_map_fill_impl( &mask->map[0] );

        /* マスク不可能イベントを開ける  */
	ev_mask_unset_impl( &mask->map[0], EV_SIG_KILL); 
}

/** イベントマップ中の所定のビットがマスクされていることを確認する
    @param[in] mask 操作対象のイベントマップ
    @param[in] id  イベントID
    @retval    真  イベントマップ中の所定のビットがマスクされている
    @retval    偽  イベントマップ中の所定のビットがマスクされていない
 */
bool
ev_mask_test(event_mask *mask, event_no id) {

	return ev_mask_test_impl(&mask->map[0], id);
}

/** イベントマスクが空であることを確認する
    @param[in] mask 操作対象のイベントマスク
    @param[in] id  イベントID
    @retval    真  イベントマップが空
    @retval    偽  イベントマップが空でない
 */
bool
ev_mask_empty(event_mask *mask) {

	return ev_mask_empty_impl( &mask->map[0] );
}

/** イベントマスク中の所定のイベントをマスクしたイベントマスクを返却する
    @param[in] mask 操作対象のマスク
    @param[in] id   クリアするイベント
 */
void
ev_mask_set(event_mask *mask, event_no id) {

	if ( id == EV_SIG_KILL)
		return;  /* マスク不可能イベント  */

	ev_mask_set_impl(&mask->map[0], id);
}

/** イベントマスク中の所定のイベントをマスクしたイベントマスクを返却する
    @param[in] mask 操作対象のマスク
    @param[in] id   クリアするイベント
 */
void
ev_mask_unset(event_mask *mask, event_no id) {

	ev_mask_unset_impl(&mask->map[0], id);
}

/** イベントマスクの排他的論理和を取る
    @param[in]  mask1 操作対象のマスク1
    @param[in]  mask2 操作対象のマスク1
    @param[out] maskp 演算後のマスクの格納アドレス
 */
void
ev_mask_xor(event_mask *mask1, event_mask *mask2, event_mask *maskp) {
	int i;

	for( i = 0; EV_MAP_LEN > i; ++i ){

		maskp->map[i] = mask1->map[i] ^ mask2->map[i];
	}
}

/** イベントマスクの論理積を取る
    @param[in]  mask1 操作対象のマスク1
    @param[in]  mask2 操作対象のマスク1
    @param[out] maskp 演算後のマスクの格納アドレス
 */
void
ev_mask_and(event_mask *mask1, event_mask *mask2, event_mask *maskp) {
	int i;


	for( i = 0; EV_MAP_LEN > i; ++i ){

		maskp->map[i] = mask1->map[i] & mask2->map[i];
	}
}


