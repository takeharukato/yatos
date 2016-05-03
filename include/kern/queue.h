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

/** queue definition
 */
typedef struct _queue{
	struct _list *prev;       /*<  Previous link  */
	struct _list *next;       /*<  Next link  */
}queue;

#define __QUEUE_INITIALIZER(_que)			\
	{						\
		.prev = (struct _list *)(_que),		\
		.next = (struct _list *)(_que),		\
	}

void queue_add(struct _queue *, list *);
void queue_add_top(struct _queue *, list *);
list *queue_ref_top(struct _queue *);
list *queue_get_top(struct _queue *);
list *queue_ref_last(struct _queue *);
list *queue_get_last(struct _queue *);
bool queue_is_empty(struct _queue *);
void queue_init(struct _queue *);
#endif  /*  __KERN_QUEUE_H  */
