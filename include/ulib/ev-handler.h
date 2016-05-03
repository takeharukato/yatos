/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Event handler relevant definitions                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_ULIB_EVENTS_H)
#define _ULIB_EVENTS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/async-event.h>

#include <ulib/yatos-ulib.h>

#define is_ev_non_catchable_event(id) \
	( ( (id) == EV_SIG_KILL ) || ( (id) == 0 ) )

/** イベントハンドラの型
 */
typedef void (*ev_handler)(event_id  , struct _evinfo *, void *);

#define EV_HDLR_DFL	((ev_handler)0)	/*< デフォルト動作 */
#define EV_HDLR_IGN	((ev_handler)1)	/*< 無視 */

/** プロセスのイベントハンドラ
 */
typedef struct _ev_handler_array{
	ev_handler  handlers[EV_NR_EVENT];
}ev_handler_array;

void __yatos_user_event_handler_init(void);
int yatos_register_user_event_handler(event_id _id, ev_handler _handler);
void __yatos_ulib_invoke_handler(event_id _id, evinfo *_info, event_frame *_evf);
#endif  /*  __ULIB_EVENTS_H  */
