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
#include <kern/thread-reaper.h>
#include <kern/sched.h>
#include <kern/proc.h>

#include <kern/tst-progs.h>

#include <hal/prepare.h>
#include <hal/kernlayout.h>
#include <hal/kconsole.h>
#include <hal/segment.h>
#include <hal/pgtbl.h>
#include <hal/arch-cpu.h>

extern karch_info  *_refer_boot_info(void);

static const char *environ[] = {
	"TERM=yatos",
	NULL
};

void
proc_create_test(void) {
	int i;
	int rc;
	proc *p;
	grub_mod  *mod;
	karch_info  *info = _refer_boot_info();
	
	for( i = 0; info->nr_mod > i; ++i) {
		
		mod = &info->modules[i];
		rc = proc_create(&p, 0, mod->param, environ, 
		    (void *)PHY_TO_KERN_STRAIGHT(mod->start));
		kassert(rc == 0);

		rc = proc_start(p);
		kassert(rc == 0);
	}
}
