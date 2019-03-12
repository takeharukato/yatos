/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*   list relevant definitions                                        */
/*                                                                    */
/**********************************************************************/
#if !defined(__KERN_LIST_H)
#define __KERN_LIST_H

#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>

/** 構造体に埋め込まれたリスト構造体のアドレスから構造体へのポインタを得る
    @param[in] p リスト構造体のアドレス
    @param[in] t リスト構造体を埋め込んでいる構造体の型
    @param[in] m リスト構造体を埋め込んでいる構造体中のリストメンバの名前
 */
#define CONTAINER_OF(p, t, m)			\
	( (t *)( ( (void *)(p) ) - ( (void *)( &( ((t *)(0))->m ) ) ) ) )

/** リスト構造体
 */
typedef struct _list{
	struct _list *prev;       /**<  前の要素へのポインタ  */
	struct _list *next;       /**<  後の要素へのポインタ  */
}list;

void list_del(struct _list *);
void list_init(struct _list *);
int  list_not_linked(struct _list *);
#endif  /*  __KERN_LIST_H  */
