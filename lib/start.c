/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Userlland event handler  routines                                 */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <ulib/yatos-ulib.h>
#include <ulib/event-svc.h>
#include <ulib/thread-svc.h>
#include <ulib/ev-handler.h>

extern int main(int _argc, char *_argv[]);

char **environ;

void
_start(int argc, char *argv[], char **envp) {
	int rc;

	_clear_bss();

	environ = envp;
	__yatos_user_event_handler_init();
	_yatos_register_common_event_handler();

	rc = main(argc, argv);

	yatos_thread_exit(rc);
}

