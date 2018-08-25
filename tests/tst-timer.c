/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Time test routines                                                */
/*                                                                    */
/**********************************************************************/

#include <stddef.h>
#include <stdint.h>

#include <kern/config.h>
#include <kern/assert.h>
#include <kern/string.h>
#include <kern/kprintf.h>
#include <kern/thread.h>
#include <kern/sched.h>
#include <kern/proc.h>
#include <kern/timer.h>

#include <kern/tst-progs.h>

typedef struct _tst_sync_obj{
	spinlock lock;
	sync_obj obj;
}tst_sync_obj;

#define TST_SYNC_OBJECT_INITIALIZER(tso)			\
	{							\
	.lock = __SPINLOCK_INITIALIZER,				\
	.obj = __SYNC_OBJECT_INITIALIZER(tso.obj, SYNC_WAKE_FLAG_ALL, THR_TSTATE_WAIT), \
	}

/*  同期オブジェクト  */
static tst_sync_obj sobj1 = TST_SYNC_OBJECT_INITIALIZER(sobj1);
static tst_sync_obj sobj2 = TST_SYNC_OBJECT_INITIALIZER(sobj2);
static tst_sync_obj sobj3 = TST_SYNC_OBJECT_INITIALIZER(sobj3);

int
kthreadA(void __attribute__ ((unused)) *arg) {
	sync_reason res;

	kprintf(KERN_INF, "ThreadA:tid=%d thread=%p\n", 
	    current->tid, current);

	kprintf(KERN_INF, "ThreadA:wait 1000ms\n");
	res = tim_wait(1000);
	if ( res == SYNC_WAI_TIMEOUT ) {
		
		kprintf(KERN_INF, "ThreadA:time out OK\n");
	} else {
		
		kprintf(KERN_INF, "ThreadA:time out NG res=%d\n", res);
	}

	kprintf(KERN_INF, "ThreadA:wait on sobj1 800ms\n");

	spinlock_lock(&(sobj1.lock));
	res = tim_wait_obj(&sobj1.obj, 800, &sobj1.lock);
	spinlock_unlock(&(sobj1.lock));

	if ( res == SYNC_WAI_TIMEOUT ) {

		kprintf(KERN_INF, "ThreadA:time out OK\n");
	} else {
		
		kprintf(KERN_INF, "ThreadA:time out NG res=%d\n", res);
	}

	kprintf(KERN_INF, "ThreadA:wake up threads on sobj2.\n");
	sync_wake(&sobj2.obj, SYNC_WAI_RELEASED);

	kprintf(KERN_INF, "ThreadA:wait with time out on sobj3 2000ms\n");

	spinlock_lock(&(sobj3.lock));
	res = tim_wait_obj(&sobj3.obj, 2000, &sobj3.lock);
	spinlock_unlock(&(sobj3.lock));

	if ( res == SYNC_WAI_RELEASED ) {
		
		kprintf(KERN_INF, "ThreadA:wake up by threadB OK\n");
	} else {
		
		kprintf(KERN_INF, "ThreadA:time out NG res=%d\n", res);
	}
	
	kprintf(KERN_INF, "ThreadA:Exit ThreadA with return 0\n");

	return 0;
}

int
kthreadB(void __attribute__ ((unused)) *arg) {
	sync_reason res;

	kprintf(KERN_INF, "ThreadB: tid=%d thread=%p\n", 
	    current->tid, current);

	kprintf(KERN_INF, "ThreadB: wait on sobj2\n");
	spinlock_lock(&(sobj2.lock));
	res = sync_wait( &sobj2.obj, &(sobj2.lock));
	spinlock_unlock(&(sobj2.lock));
	if ( res == SYNC_WAI_RELEASED)
		kprintf(KERN_INF, "ThreadB: wake up from sobj2 OK\n");
	else
		kprintf(KERN_INF, "ThreadB: wake up from sobj2 NG res=%d\n", res);

	kprintf(KERN_INF, "ThreadB: wake up threads on sobj3\n");
	sync_wake(&sobj3.obj, SYNC_WAI_RELEASED);

	kprintf(KERN_INF, "ThreadB: Exit ThreadB with return 0\n");

	return 0;
}

void
timer_test(void) {
	int              rc;
	thread *thrA, *thrB;

	rc = thr_new_thread(&thrA);
	kassert( rc == 0 );

	rc = thr_new_thread(&thrB);
	kassert( rc == 0 );

	rc = thr_create_kthread(thrA, 0, THR_FLAG_NONE,
	    THR_INVALID_TID, kthreadA, (void *)"ThreadA");
	kassert( rc == 0 );

	rc = thr_create_kthread(thrB, 0, THR_FLAG_NONE,
	    THR_INVALID_TID, kthreadB, (void *)"ThreadB");
	kassert( rc == 0 );

	rc = thr_start(thrA, current->tid);
	kassert( rc == 0 );

	rc = thr_start(thrB, current->tid);
	kassert( rc == 0 );
}

