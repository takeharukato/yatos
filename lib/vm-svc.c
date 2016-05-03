/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  process system call routines                                      */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <ulib/yatos-ulib.h>
#include <ulib/vm-svc.h>
#include <ulib/lpc-svc.h>
#include <ulib/utils.h>

void *
yatos_vm_sbrk(intptr_t increment) {
	int               rc;
	msg_body         msg;
	vm_service   *pmsg;
	vm_sys_sbrk  *sbrk;

	pmsg= &msg.vm_msg;
	sbrk = &pmsg->vm_service_calls.sbrk;

	memset( &msg, 0, sizeof(msg_body) );

	pmsg->req = VM_SERV_REQ_SBRK;
	sbrk->inc = increment;

	rc = yatos_lpc_send_and_reply( ID_RESV_VM, &msg );
	if ( rc != 0 ) {

		set_errno(rc);
		return NULL;
	}

	if ( pmsg->rc != 0 ) {

		set_errno(pmsg->rc);
		return NULL;
	}

	return sbrk->old_heap_end;
}

