/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  thread test routines                                              */
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

#include <kern/tst-progs.h>

int
kthreadA(void __attribute__ ((unused)) *arg) {

	while(1) {

		kprintf(KERN_INF, "ThreadA:tid=%d thread=%p\n", 
		    current->tid, current);
		thr_yield();
	}

	kprintf(KERN_INF, "ExitThreadA\n");
	thr_exit(0);

	return 0;
}

int
kthreadB(void __attribute__ ((unused)) *arg) {

	kprintf(KERN_INF, "ThreadB:tid=%d thread=%p\n", 
	    current->tid, current);
	thr_yield();
	kprintf(KERN_INF, "Exit ThreadB with return -1\n");

	return -1;
}

int
kthreadC(void __attribute__ ((unused)) *arg) {

	while(1) {

		kprintf(KERN_INF, "ThreadC:tid=%d thread=%p\n", 
		    current->tid, current);
		thr_yield();
		kprintf(KERN_INF, "Exit ThreadC with thr_exit(-1)\n");
		thr_exit(-1);
	}

	return -1;
}

int
kthreadD(void __attribute__ ((unused)) *arg) {

	while(1) {

		kprintf(KERN_INF, "ThreadD:tid=%d thread=%p\n", 
		    current->tid, current);
		thr_yield();
	}
}

void
thread_test(void) {
	int              rc;
	thread *thrA, *thrB;
	thread *thrC, *thrD;

	rc = thr_new_thread(&thrA);
	kassert( rc == 0 );

	rc = thr_new_thread(&thrB);
	kassert( rc == 0 );

	rc = thr_new_thread(&thrC);
	kassert( rc == 0 );

	rc = thr_new_thread(&thrD);
	kassert( rc == 0 );

	rc = thr_create_kthread(thrA, 0, THR_FLAG_NONE, 
	    THR_INVALID_TID, kthreadA, (void *)"ThreadA");
	kassert( rc == 0 );

	rc = thr_create_kthread(thrB, 0, THR_FLAG_NONE, 
	    THR_INVALID_TID, kthreadB, (void *)"ThreadB");
	kassert( rc == 0 );

	rc = thr_create_kthread(thrC, 0, THR_FLAG_NONE, 
	    THR_INVALID_TID, kthreadC, (void *)"ThreadC");
	kassert( rc == 0 );

	rc = thr_create_kthread(thrD, 0, THR_FLAG_NONE,
	    THR_INVALID_TID, kthreadD, (void *)"ThreadD");
	kassert( rc == 0 );

	rc = thr_start(thrA, current->tid);
	kassert( rc == 0 );

	rc = thr_start(thrB, current->tid);
	kassert( rc == 0 );

	rc = thr_start(thrC, current->tid);
	kassert( rc == 0 );

	rc = thr_start(thrD, current->tid);
	kassert( rc == 0 );
}

