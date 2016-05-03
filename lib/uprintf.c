/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  printf routines                                                   */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <limits.h>

#include <ulib/libyatos.h>

struct _string_stream{
	size_t    pos;
	size_t    max;
	char    *buff;
};

static void
init_string_stream(struct _string_stream *sbuf, char *buf, size_t max){

	if (sbuf == NULL)
		return;

	sbuf->pos = 0;
	sbuf->buff = buf;
	sbuf->max = max;
}

static int 
string_stream_putc(char c, void *argp){
	struct _string_stream *sbuf;

	sbuf = (struct _string_stream *)argp;

	if (sbuf->pos == sbuf->max)
		return 0;

	sbuf->buff[sbuf->pos] = c;
	++sbuf->pos;

	return 1;
}

/** snprintf関数
 */
int
yatos_snprintf(char *buf, size_t size, const char *fmt,...){
	va_list args;
	int rc;
	struct _string_stream sbuf;

	init_string_stream(&sbuf, buf, size);

	va_start(args, fmt);
	rc = doprintf(string_stream_putc, &sbuf, fmt, args);
	va_end(args);
	
	return rc;
}

/** 簡易版printf関数
 */
int
yatos_printf(const char *fmt,...) {
	va_list                  args;
	int                        rc;
	char buf[YATOS_PRINTF_BUFSIZ];
	struct _string_stream    sbuf;
	msg_body                  msg;
	pri_string_msg          *pmsg;

	init_string_stream(&sbuf, buf, YATOS_PRINTF_BUFSIZ);

	va_start(args, fmt);
	rc = doprintf(string_stream_putc, &sbuf, fmt, args);
	va_end(args);

	pmsg= &msg.sys_pri_dbg_msg;
	memset( &msg, 0, sizeof(msg_body) );

	pmsg->req = 0;
	pmsg->msg = buf;
	pmsg->len = strlen(pmsg->msg) + 1;

	yatos_lpc_send_and_reply(ID_RESV_DBG_CONSOLE, &msg);

	return rc;
}
