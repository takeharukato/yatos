/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Event allocation  routines                                        */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/errno.h>
#include <kern/spinlock.h>
#include <kern/proc.h>
#include <kern/async-event.h>
#include <kern/queue.h>
#include <kern/list.h>
#include <kern/page.h>

/** イベントノードを割当てる
    @param[in]  id     割り当てるイベントID
    @param[out] nodep  イベントノードアドレス格納先
    @retval     0      割り当て成功
    @retval    -ENOMEM メモリ不足により割当て失敗
 */
int
ev_alloc_node(event_id id, event_node **nodep) {
	event_node *node;
	evinfo     *info;

	kassert( nodep != NULL );

	node = kmalloc(sizeof(event_node), KMALLOC_NORMAL);
	if ( node == NULL )
		return -ENOMEM;

	memset(node, 0, sizeof(event_node) );

	list_init( &node->link );
	info = &node->info;
	
	info->no = id;
	if ( current->p == hal_refer_kernel_proc() )
		info->code = EV_SIG_SI_KERNEL;
	else
		info->code = EV_SIG_SI_USER;

	*nodep = node;

	return 0;
}
