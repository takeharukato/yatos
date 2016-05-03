/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Local process communication relevant definitions                  */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_THREAD_LPC_H)
#define  _KERN_THREAD_LPC_H 

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

#define LPC_ENDPOINT_ANY   (0)   /*< 全エンドポイントを指定(THR_IDLE_TIDと同値) */

typedef struct _message_queue{
	spinlock          lock;  /*< メッセージキューのロック                        */
	queue              que;  /*< メッセージキュー                                */
	sync_obj   wait_sender;  /*< 受付け待ちオブジェクト(受信者をキューイング)    */
	sync_obj wait_reciever;  /*< 送信開始待ちオブジェクト(送信者をキューイング)  */
}message_queue;

typedef struct _message{
	list                link;  /*< メッセージキューへのリンク                        */
	endpoint             src;  /*< 送信元エンドポイント                              */
	endpoint            dest;  /*< 送信先エンドポイント                              */
	sync_obj wait_completion;  /*< 送信完了待ちオブジェクト((送信者をキューイング))  */
	message_body         msg;  /*< メッセージの本体                                  */
}message;

int lpc_send(endpoint _dest, lpc_tmout _tmout, message *_m);
int lpc_recv(endpoint _src, lpc_tmout _tmout, message *_m);
int lpc_send_recv(endpoint _dest, lpc_tmout _tmout, message *_m);
#endif  /*  _KERN_THREAD_LPC_H   */
