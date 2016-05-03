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

/** obtain struct pointer from embedded list member address.
    @param[in] p address of list structure
    @param[in] t type of struct
    @param[in] m list member name in the structure
 */
#define CONTAINER_OF(p, t, m)			\
	( (t *)( ( (void *)(p) ) - ( (void *)( &( ((t *)(0))->m ) ) ) ) )

/** list entry
 */
typedef struct _list{
	struct _list *prev;       /*<  Previous pointer  */
	struct _list *next;       /*<  Next pointer  */
}list;


void list_del(struct _list *);
void list_init(struct _list *);
int  list_not_linked(struct _list *);
#endif  /*  __KERN_LIST_H  */
