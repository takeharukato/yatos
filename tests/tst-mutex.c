/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  mutex test routines                                               */
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
#include <kern/mutex.h>

#include <kern/tst-progs.h>

mutex mtx1, mtx2;

int
kthreadA(void __attribute__ ((unused)) *arg) {

	kprintf(KERN_INF, "ThreadA:tid=%d thread=%p mtx lock mtx1\n", 
	    current->tid, current);
	mutex_lock(&mtx1);
	thr_yield();
	kprintf(KERN_INF, "ThreadA:tid=%d thread=%p mtx unlock mtx1\n", 
	    current->tid, current);
	mutex_unlock(&mtx1);

	kprintf(KERN_INF, "ThreadA:tid=%d thread=%p mtx lock mtx2\n", 
	    current->tid, current);
	mutex_lock(&mtx2);

	kprintf(KERN_INF, "ThreadA:tid=%d thread=%p mtx unlock mtx2\n", 
	    current->tid, current);
	mutex_unlock(&mtx2);

	kprintf(KERN_INF, "ExitThreadA\n");
	thr_exit(0);

	return 0;
}

int
kthreadB(void __attribute__ ((unused)) *arg) {

	kprintf(KERN_INF, "ThreadB:tid=%d thread=%p mtx lock mtx2\n", 
	    current->tid, current);
	mutex_lock(&mtx2);

	thr_yield();

	kprintf(KERN_INF, "ThreadB:tid=%d thread=%p mtx lock mtx1\n", 
	    current->tid, current);
	mutex_lock(&mtx1);

	kprintf(KERN_INF, "ThreadB:tid=%d thread=%p mtx unlock mtx2\n", 
	    current->tid, current);
	mutex_unlock(&mtx2);

	kprintf(KERN_INF, "ThreadB:tid=%d thread=%p mtx unlock mtx1\n", 
	    current->tid, current);
	mutex_unlock(&mtx1);

	kprintf(KERN_INF, "Exit ThreadB with return -1\n");
	return -1;
}

void
mutex_test(void) {
	int              rc;
	thread *thrA, *thrB;

	mutex_init(&mtx1, MTX_FLAG_EXCLUSIVE);
	mutex_init(&mtx2, MTX_FLAG_EXCLUSIVE);

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

