/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  queue operation definitions                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(__KERN_QUEUE_H)
#define __KERN_QUEUE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/list.h>

/** キュー構造体
 */
typedef struct _queue{
	struct _list *prev;       /*<  最後尾の要素へのポインタ  */
	struct _list *next;       /*<  先頭要素へのポインタ      */
}queue;

/** キュー構造体の初期化子
 */
#define __QUEUE_INITIALIZER(_que)			\
	{						\
		.prev = (struct _list *)(_que),		\
		.next = (struct _list *)(_que),		\
	}

/** キューの中を順に探索するマクロ
    @param[in] _itr    イテレータ (list構造体のポインタの名前)
    @param[in] _que    キューへのポインタ
 */
#define queue_for_each(_itr, _que)				   \
	for((_itr) = queue_ref_top((struct _queue *)(_que));	   \
	    (_itr) != ((struct _list *)(_que));			   \
	    (_itr) = (_itr)->next) 

/** キューの中を順に探索するマクロ (ループ内でのキューの変更がある場合)
    @param[in] _itr  イテレータ (list構造体のポインタの名前)
    @param[in] _que  キューへのポインタ
    @param[in] _np   次の要素を指し示しておくポインタ
 */
#define queue_for_each_safe(_itr, _que, _np)				\
	for((_itr) = queue_ref_top((struct _queue *)(_que)), (_np) = (_itr)->next; \
	    (_itr) != ((struct _list *)(_que));				\
	    (_itr) = (_np), (_np) = (_itr)->next ) 

/** キューの中を逆順に探索するマクロ
    @param[in] _itr    イテレータ (list構造体のポインタの名前)
    @param[in] _que   キューへのポインタ
 */
#define queue_reverse_for_each(_itr, _que)		      \
	for((_itr) = queue_ref_last((struct _queue *)(_que)); \
	    (_itr) != (struct _list *)(_que);		      \
	    (_itr) = (_itr)->prev ) 

/** キューの中を逆順に探索するマクロ (ループ内でのキューの変更がある場合)
    @param[in] _itr  イテレータ (list構造体のポインタの名前)
    @param[in] _que  キューへのポインタ
    @param[in] _np   次の要素を指し示しておくポインタ
 */
#define queue_reverse_for_each_safe(_itr, _que, _np)			\
	for((_itr) = queue_ref_last((struct _queue *)(_que)), (_np) = (_itr)->prev; \
	    (_itr) != (struct _list *)(_que);				\
	    (_itr) = (_np), (_np) = (_itr)->prev ) 

/** リストを埋め込んだデータ構造を順にたどって要素を見つけ出す
    @param[in] _que     キューへのポインタ
    @param[in] _type    リストを埋め込んだデータ構造への型
    @param[in] _member  埋め込んだデータ構造のリストメンバの名前
    @param[in] _keyp    比較するキー(リストを埋め込んだデータ構造)へのポインタ
    @param[in] _cmp     比較関数へのポインタ
    @param[out] _elemp  見つけた要素(リストを埋め込んだデータ構造)を指し示すポインタのアドレス
 */
#define queue_find_element(_que, _type, _member, _keyp, _cmp, _elemp) do{ \
		struct _list *_lp;					\
		_type  *_elem_ref;					\
									\
		*((_type **)(_elemp)) = NULL;				\
									\
		queue_for_each((_lp), (_que)) {				\
									\
			(_elem_ref) =					\
				CONTAINER_OF(_lp, _type, _member);	\
									\
			if ( _cmp((_keyp), (_elem_ref)) == 0 ) {	\
									\
				*((_type **)(_elemp)) =	(_elem_ref);	\
				break;					\
			}						\
		}							\
	}while(0)

/** リストを埋め込んだデータ構造を逆順にたどって要素を見つけ出す
    @param[in] _que     キューへのポインタ
    @param[in] _type    リストを埋め込んだデータ構造への型
    @param[in] _member  埋め込んだデータ構造のリストメンバの名前
    @param[in] _keyp    比較するキー(リストを埋め込んだデータ構造)へのポインタ
    @param[in] _cmp     比較関数へのポインタ
    @param[out] _elemp  見つけた要素(リストを埋め込んだデータ構造)を指し示すポインタのアドレス
 */
#define queue_reverse_find_element(_que, _type, _member, _keyp, _cmp, _elemp) do{ \
		struct _list *_lp;					\
		_type  *_elem_ref;					\
									\
		*((_type **)(_elemp)) = NULL;				\
									\
		queue_reverse_for_each((_lp), (_que)) {			\
									\
			(_elem_ref) =					\
				CONTAINER_OF(_lp, _type, _member);	\
									\
			if ( _cmp((_keyp), (_elem_ref)) == 0 ) {	\
									\
				*((_type **)(_elemp)) =	(_elem_ref);	\
				break;					\
			}						\
		}							\
	}while(0)

void queue_add(struct _queue *, list *);
void queue_add_top(struct _queue *, list *);
struct _list *queue_ref_top(struct _queue *);
struct _list *queue_get_top(struct _queue *);
struct _list *queue_ref_last(struct _queue *);
struct _list *queue_get_last(struct _queue *);
bool queue_is_empty(struct _queue *);
void queue_init(struct _queue *);
#endif  /*  __KERN_QUEUE_H  */
