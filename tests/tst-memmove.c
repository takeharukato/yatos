/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  memmove test routines                                             */
/*                                                                    */
/**********************************************************************/

#include <stddef.h>
#include <stdint.h>

#include <kern/config.h>
#include <kern/assert.h>
#include <kern/string.h>
#include <kern/kprintf.h>
#include <kern/thread.h>
#include <kern/thread-reaper.h>
#include <kern/sched.h>
#include <kern/proc.h>

#include <kern/tst-progs.h>

void
test_memmove(void) {
	char    src[10];
        char    dst[10];
	char    buf[10];
        int     i1;

        for (i1 = 0; i1 < 10; i1++)  {

		    src[i1] = i1;
		    buf[i1] = i1;
	}

        memmove(dst, src, 10);
	if ( memcmp(dst, src, 10) ) {

		kprintf(KERN_INF, "memmove: Case1 not overlap case NG\n");
		return;
	}

	kprintf(KERN_INF, "memmove: Case1 not overlap case OK\n");
	memmove(buf, buf + 3, 7);

	if ( memcmp(buf, src+3, 7) ) {

		kprintf(KERN_INF, "memmove: Case2  overlap case NG\n");
		for (i1 = 0; i1 < 10; i1++)  {
			
			kprintf(KERN_INF, "[%d]",buf[i1]);
			kprintf(KERN_INF, "\n");
		}
		return;
	}
	kprintf(KERN_INF, "memmove: Case2 overlap case OK\n");
}
