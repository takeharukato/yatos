/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vsprintf routines                                                 */
/*                                                                    */
/**********************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <limits.h>

#include <kern/kprintf.h>
#include <kern/string.h>

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
int
ksnprintf(char *buf, size_t size, const char *fmt,...){
	va_list args;
	int rc;
	struct _string_stream sbuf;

	init_string_stream(&sbuf, buf, size);

	va_start(args, fmt);
	rc = doprintf(string_stream_putc, &sbuf, fmt, args);
	va_end(args);
	
	return rc;
}

int
ksprintf(char *buf, const char *fmt,...){
	va_list args;
	int rc;
	struct _string_stream sbuf;

	init_string_stream(&sbuf, buf, INT_MAX);
	va_start(args, fmt);
	rc = doprintf(string_stream_putc, &sbuf, fmt, args);
	va_end(args);
	
	return rc;
}
