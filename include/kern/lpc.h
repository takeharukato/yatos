/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Local process communication relevant definitions                  */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_LPC_H)
#define  _KERN_LPC_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>
#include <kern/queue.h>
#include <kern/thread-sync.h>
#include <kern/messages.h>
#include <kern/kresv-ids.h>

#define LPC_NON_BLOCK  (0)             /*< ノンブロック通信                    */
#define LPC_INFINITE   (-1)            /*< ブロック通信                        */
#define LPC_RECV_ANY   (ID_RESV_IDLE)  /*< 任意のスレッドからの送信を受け付け  */

struct _thread;

/** メッセージキュー
 */
typedef struct  _msg_queue{
	spinlock          lock;  /*< メッセージキューのロック                        */
	queue              que;  /*< メッセージキュー                                */
	struct _thread  *owner;  /*< キューのオーナー                                */
	sync_obj   wait_sender;  /*< 受付け待ちオブジェクト(受信者をキューイング)    */
	sync_obj wait_reciever;  /*< 送信開始待ちオブジェクト(送信者をキューイング)  */
}msg_queue;

/** メッセージ
 */
typedef struct  _msg{
	list                link;  /*< メッセージキューへのリンク                     */
	struct   _msg_queue  *qp;  /*< キューイング先のメッセージキュー               */
	endpoint             src;  /*< 送信元エンドポイント                           */
	endpoint            dest;  /*< 送信元エンドポイント                           */
	spinlock          cqlock;  /*< 送信完了待ちキューのロック                     */
	sync_obj      completion;  /*< 送信完了待ちオブジェクト(送信者をキューイング) */
	msg_body            body;  /*< メッセージの本体                               */
}msg;

void lpc_msg_queue_init(struct _msg_queue *_que);
void lpc_destroy_msg_queue(msg_queue *_que);
int lpc_send(endpoint _dest, lpc_tmout _tmout, void *_m);
int lpc_recv(endpoint _src, lpc_tmout _tmout, void *_m, endpoint *_msg_src);
int lpc_send_and_reply(endpoint _dest, void *_m);
#endif  /*  _KERN_LPC_H   */
