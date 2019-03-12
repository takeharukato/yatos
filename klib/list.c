/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  list routines                                                     */
/*                                                                    */
/**********************************************************************/
#include <kern/list.h>

/** リストノードを初期化する
    @param[in] node 初期化対象のリストノード
 */
void 
list_init(struct _list *node){

	node->prev = node->next = node;	
}

/** リストノードをキューから外す
    @param[in] node 操作対象のリストノード
 */
void
list_del(struct _list *node) {

	/*
	 * リストノードをキューから外す
	 */
	node->next->prev = node->prev;
	node->prev->next = node->next;

	list_init(node);  	/*  取り外したノードを再初期化する  */
}


/** ノードがどのキューにも接続されていないことを確認する
    @param[in] node 操作対象のリストノード
 */
int
list_not_linked(struct _list *node) {
	
	return ( ( node->prev == node ) && ( node->next == node ) );
}
