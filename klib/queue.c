/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  queue routines                                                    */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/list.h>
#include <kern/queue.h>

/** リストノードをキューの末尾に追加する
    @param[in] head 追加先キュー
    @param[in] node リストノード
 */
void
queue_add(struct _queue *head, list *node) {

	node->next = (list *)head;
	node->prev = head->prev;
	head->prev->next = node;
	head->prev = node;
}

/** リストノードをキューの先頭に追加する
    @param[in] head 追加先キュー
    @param[in] node リストノード
 */
void
queue_add_top(struct _queue *head, list *node) {

	node->next = head->next;
	node->prev = head->prev;
	head->next->prev = node;
	head->next = node;
}

/** キューの先頭ノードを参照する
    @param[in] head 調査対象キュー
    @retval 先頭リストノードのアドレス
    @note 参照するだけでリストノードの取り外しは行わない
 */
list *
queue_ref_top(struct _queue *head) {
	
	return head->next;
}

/** キューの先頭ノードを取り出す
    @param[in] head 操作対象キュー
    @retval 先頭リストノードのアドレス
 */
list *
queue_get_top(struct _queue *head) {
	list *top;

	top = queue_ref_top(head);
	list_del(top);

	return top;
}

/** キューの末尾ノードを参照する
    @param[in] head 調査対象キュー
    @retval 末尾のリストノードのアドレス
    @note 参照するだけでリストノードの取り外しは行わない
 */
list *
queue_ref_last(struct _queue *head) {
	
	return head->prev;
}

/** キューの末尾ノードを取り出す
    @param[in] head 操作対象キュー
    @retval 末尾のリストノードのアドレス
 */
list *
queue_get_last(struct _queue *head) {
	list *last;

	last = queue_ref_last(head);
	list_del(last);

	return last;
}

/** キューが空であることを確認する
    @param[in] head 調査対象キュー
 */
bool
queue_is_empty(struct _queue *head) {
	
	return  (head->prev == (list *)head) && (head->next == (list *)head);
}

/** キューを初期化する
    @param[in] head 操作対象キュー
 */
void 
queue_init(struct _queue *head) {

	head->prev = head->next = (list *)head;	
}
