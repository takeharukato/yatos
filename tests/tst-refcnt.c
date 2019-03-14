/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Test program for reference counter routines                       */
/*                                                                    */
/**********************************************************************/

#include <stddef.h>
#include <stdint.h>

#include <kern/config.h>
#include <kern/errno.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/refcount.h>

#include <kern/tst-progs.h>

/** 参照カウンタルートテスト
 */
static void
refcnt_test1(void) {
	int         rc;
	refcnt     cnt;
	refcnt_val val;

	val = 0;

	refcnt_init(&cnt);
	kprintf(KERN_INF, "Reference counter Test1-1: refcnt_init\n");

	rc = refcnt_get(&cnt, NULL);
	kprintf(KERN_INF, "Reference counter Test1-2: refcnt_get no need to obtain a value\n");
	kassert( rc == 0 );

	rc = refcnt_get(&cnt, &val);
	kprintf(KERN_INF, "Reference counter Test1-3: refcnt_get obtain a value: %d\n", (int)val);
	kassert( rc == 0 );
	kassert( val == 2 );

	rc = refcnt_put(&cnt, &val);
	kprintf(KERN_INF, "Reference counter Test1-4: refcnt_put -EBUSY case\n", (int)val);
	kassert( rc == -EBUSY );
	kassert( val == 3 );

	rc = refcnt_put(&cnt, NULL);
	kprintf(KERN_INF, "Reference counter Test1-5: refcnt_put no need to obtain a value\n");
	kassert( rc == -EBUSY );
	
	refcnt_mark_deleted(&cnt);
	kprintf(KERN_INF, "Reference counter Test1-6: mark deleted\n");

	rc = refcnt_get(&cnt, &val);
	kprintf(KERN_INF, "Reference counter Test1-7: refcnt_get deleted object val: %d\n", (int)val);
	kassert( rc == -ENOENT );
	kassert( val == 1 );

	rc = refcnt_put(&cnt, &val);
	kprintf(KERN_INF, "Reference counter Test1-8: refcnt_put to be destroyed val: %d\n", (int)val);
	kassert( val == 1 );
	kassert( rc == 0);
}

/** 参照カウンタテスト
 */
void
refcnt_test(void){

	refcnt_test1();
}
