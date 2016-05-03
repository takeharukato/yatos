/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  errno routines                                                    */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <ulib/yatos-ulib.h>
#include <ulib/event-svc.h>

int errno;

void
set_errno(syscall_res_type res) {

	if ( res < 0 )
		errno = (int)-res;
	else
		errno = 0;
}
