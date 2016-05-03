/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  kernel printf definitions                                         */
/*                                                                    */
/**********************************************************************/
#if !defined(__KERN_KPRINTF_H)
#define __KERN_KPRINTF_H

#include <stdint.h>
#include <stddef.h>

#include <stdarg.h>

#include <kern/spinlock.h>

#include <hal/kconsole.h>

#define KERN_CRI   (0)
#define KERN_ERR   (1)
#define KERN_WAR   (2)
#define KERN_INF   (3)
#define KERN_DBG   (4)

typedef struct _kprint_info{
	spinlock   lock;
	int debug_level;
}kprint_info;

#define __KERN_PRINT_INFO_INITIALIZER				\
	{							\
		.lock =  __SPINLOCK_INITIALIZER,		\
		.debug_level  = KERN_DBG,		        \
	}

int kvsnprintf(char *, size_t , const char *, va_list );
int ksprintf(char *, const char *,...);
int ksnprintf(char *, size_t , const char *,...);
int kprintf(int _lvl, const char *fmt,...);
int doprintf(int (*___putc)(char __c, void *__argp), void *_argp, const char *_fmt, va_list _args);
#endif  /*  __KERN_KPRINTF_H  */
