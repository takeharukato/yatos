/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  printf routines                                                   */
/*                                                                    */
/**********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include <kern/config.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/kprintf.h>
#include <kern/spinlock.h>

static kprint_info kprint_state = __KERN_PRINT_INFO_INITIALIZER;

static int 
__kernel_putc(char c, void *argp __attribute__ ((unused)) ){

	if ( c != 0 )
		kconsole_putc((int)c);

	return 1;
}

int
kprintf(int lvl, const char *fmt,...){
	va_list args;
	intrflags flags;
	int rc;

	raw_spinlock_lock_disable_intr(&(kprint_state.lock), &flags);
	if (kprint_state.debug_level < lvl) {

		rc = 0;
		goto out;
	}

	va_start(args, fmt);
	rc = doprintf(__kernel_putc, NULL, fmt, args);
	va_end(args);
	
out:
	raw_spinlock_unlock_restore_intr(&(kprint_state.lock), &flags);

	return rc;
}
