/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual Memory service routines                                   */
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
#include <kern/thread.h>
#include <kern/vm.h>
#include <kern/lpc.h>
#include <kern/kname-service.h>
#include <kern/proc-service.h>
#include <kern/page.h>

static thread *vm_service_thr;

//#define DEBUG_VM_SERVICE

#define MSG_PREFIX "VM:"

/** sbrkシステムコールの実処理
    @param[in] sbrk    sbrkメッセージ
    @param[in] src     呼出元エンドポイント
    @retval    0       正常に更新した
    @retval   -ENOMEM  メモリ不足により更新に失敗した
 */
static int
handle_sbrk(vm_sys_sbrk *sbrk,endpoint src) {
	int          rc;
	thread     *thr;
	void   *cur_end;
	void   *new_end;

	thr = thr_find_thread_by_tid(src);
	if ( thr == NULL ) 
		return -ENOENT;

	rc = proc_expand_heap(thr->p, NULL, &cur_end);
	if ( rc != 0 )
		return rc;

	if ( sbrk->inc == 0 ) {
		
		rc = 0;
		goto success_out;
	}

	new_end = ( PAGE_ALIGNED( (uintptr_t)(cur_end + sbrk->inc) ) ) ? 
		( (void *)( cur_end + sbrk->inc ) )  :
		( (void *)PAGE_NEXT( (uintptr_t)(cur_end + sbrk->inc) ) );

	if ( new_end < thr->p->heap->start )
		return -EINVAL;

	rc = proc_expand_heap(thr->p, new_end, &cur_end);
	if ( rc != 0 )
		return rc;

success_out:
	if ( rc == 0 ) 
		sbrk->old_heap_end = cur_end;

	return 0;
}

/** プロセスサービス処理部
    @param[in] arg スレッド引数(未使用)
 */
static int
handle_vm_service(void __attribute__ ((unused)) *arg) {
	msg_body              msg;
	vm_service          *smsg;
	vm_sys_sbrk         *sbrk;
	endpoint              src;
	int                    rc;

	rc = kns_register_kernel_service(ID_RESV_NAME_VM);
	kassert( rc == 0 );
	
	while(1) {

		memset( &msg, 0, sizeof(msg_body) );
		smsg  = &msg.vm_msg;

		rc = lpc_recv(LPC_RECV_ANY, LPC_INFINITE, &msg, &src);
		kassert( rc == 0 );

#if defined(DEBUG_VM_SERVICE)
		kprintf(KERN_INF, MSG_PREFIX "tid=%d thread=%p (src, req)=(%d, %d)\n", 
		    current->tid, current, src, smsg->req);
#endif  /*  DEBUG_VM_SERVICE  */

		switch( smsg->req ) {
		case VM_SERV_REQ_SBRK:

			sbrk=&smsg->vm_service_calls.sbrk;
			smsg->rc = handle_sbrk(sbrk, src);
			
			rc = lpc_send(src, LPC_INFINITE, &msg);
			kassert( rc == 0 );
			break;
		default:
			smsg->rc = -ENOSYS;
			rc = lpc_send(src, LPC_INFINITE, &msg);
			kassert( rc == 0 );
			break;
		}
	}

	return 0;
}

/** プロセスサービスの初期化
 */
void
vm_service_init(void) {
	int              rc;

	rc = thr_new_thread(&vm_service_thr);
	kassert( rc == 0 );

	rc = thr_create_kthread(vm_service_thr, KSERV_KSERVICE_PRIO, THR_FLAG_NONE,
	    ID_RESV_VM, handle_vm_service, (void *)ID_RESV_NAME_VM);
	kassert( rc == 0 );

	rc = thr_start(vm_service_thr, current->tid);
	kassert( rc == 0 );

}
