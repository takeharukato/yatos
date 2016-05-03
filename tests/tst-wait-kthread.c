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

int kthreadB(void *arg);
int kthreadC(void *arg);
int kthreadD(void *arg);
int kthreadE(void *arg);

int
kthreadA(void __attribute__ ((unused)) *arg) {
	int              rc;
	thread        *thrB;
	thread        *thrC;
	thread        *thrD;
	thread        *thrE;
	tid        exit_tid;
	exit_code  child_rc;

	kprintf(KERN_INF, "ThreadA:tid=%d thread=%p\n", 
		    current->tid, current);

	rc = thr_new_thread(&thrB);
	kassert( rc == 0 );

	rc = thr_new_thread(&thrC);
	kassert( rc == 0 );

	rc = thr_new_thread(&thrD);
	kassert( rc == 0 );

	rc = thr_new_thread(&thrE);
	kassert( rc == 0 );

	rc = thr_create_kthread(thrB, 0, THR_FLAG_JOINABLE, 
	    THR_INVALID_TID, kthreadB, (void *)"ThreadB");
	kassert( rc == 0 );

	rc = thr_create_kthread(thrC, 0, THR_FLAG_JOINABLE, 
	    THR_INVALID_TID, kthreadC, (void *)"ThreadC");
	kassert( rc == 0 );

	rc = thr_create_kthread(thrD, 0, THR_FLAG_JOINABLE, 
	    THR_INVALID_TID, kthreadD, (void *)"ThreadD");
	kassert( rc == 0 );

	rc = thr_create_kthread(thrE, 0, THR_FLAG_JOINABLE, 
	    THR_INVALID_TID, kthreadE, (void *)"ThreadE");
	kassert( rc == 0 );

	rc = thr_start(thrB, current->tid);
	kassert( rc == 0 );

	rc = thr_start(thrC, current->tid);
	kassert( rc == 0 );

	rc = thr_start(thrD, current->tid);
	kassert( rc == 0 );

	rc = thr_start(thrE, current->tid);
	kassert( rc == 0 );

	thr_yield();
#if defined(ENABLE_JOINABLE_KTHREAD)

	kprintf(KERN_INF, "ThreadA try to wait Any Thread \n");
	rc = thr_wait(thrB->tid, THR_WAIT_ANY, &exit_tid, &child_rc);
	kprintf(KERN_INF, "ThreadA wait Any Thread child-rc=%d\n", child_rc);

	kprintf(KERN_INF, "ThreadA try to wait ThreadD(id=%d) \n", thrD->tid);
	rc = thr_wait(thrD->tid, THR_WAIT_ID, &exit_tid, &child_rc);
	kprintf(KERN_INF, "ThreadA wait ThreadD child-rc=%d\n", child_rc);

	kprintf(KERN_INF, "ThreadA try to wait same procs \n");
	rc = thr_wait(thrC->tid, THR_WAIT_PROC, &exit_tid, &child_rc);
	kprintf(KERN_INF, "ThreadA wait ThreadD child-rc=%d\n", child_rc);
#else
	kprintf(KERN_INF, "ThreadA: You need to make ENABLE_JOINABLE_KTHREAD "
	    "in kern/thread.h defined to run this program.\n");
	child_rc = 0;
	exit_tid = 0;
#endif  /*  ENABLE_JOINABLE_KTHREAD  */

	kprintf(KERN_INF, "Exit ThreadA\n");
	/*  ThreadEをwait せずに終了  */
	thr_exit(0);

	return (child_rc + exit_tid);  /*  コンパイラの警告を避けるためにchild_rc + exit_tidで終了  */
}

int
kthreadB(void __attribute__ ((unused)) *arg) {

	kprintf(KERN_INF, "ThreadB:tid=%d thread=%p\n", 
	    current->tid, current);
	kprintf(KERN_INF, "Exit ThreadB with return 1\n");

	thr_exit(1);
	return 0;
}

int
kthreadC(void __attribute__ ((unused)) *arg) {

	kprintf(KERN_INF, "ThreadC:tid=%d thread=%p\n", 
	    current->tid, current);
	kprintf(KERN_INF, "Exit ThreadC with return 2\n");

	thr_exit(2);
	return 0;
}

int
kthreadD(void __attribute__ ((unused)) *arg) {

	kprintf(KERN_INF, "ThreadD:tid=%d thread=%p\n", 
	    current->tid, current);
	kprintf(KERN_INF, "Exit ThreadD with return 3\n");

	thr_exit(3);
	return 0;
}

int
kthreadE(void __attribute__ ((unused)) *arg) {

	kprintf(KERN_INF, "ThreadE:tid=%d thread=%p\n", 
	    current->tid, current);
	kprintf(KERN_INF, "Exit ThreadE with return 4\n");

	thr_exit(4);
	return 0;
}

void
wait_test(void) {
	int              rc;
	thread        *thrA;

	rc = thr_new_thread(&thrA);
	kassert( rc == 0 );

	rc = thr_create_kthread(thrA, 0, THR_FLAG_NONE, 
	    THR_INVALID_TID, kthreadA, (void *)"ThreadA");
	kassert( rc == 0 );

	rc = thr_start(thrA, current->tid);
	kassert( rc == 0 );
}

