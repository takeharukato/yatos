/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  service relevant definitions                                      */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_XXXX_SERVICE_H)
#define  _KERN_XXXX_SERVICE_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>

typedef struct _ZZZZ_sys_SNAME{
}XXXX_sys_SNAME;
typedef struct _YYYY_service{
	int    req;	
	int     rc;
	union _proc_service_calls{
		ZZZZ_sys_SNAME SNAME;
	}YYYY_service_calls;
}YYYY_service;

void YYYY_service_init(void);
#endif  /*  _KERN_XXXX_SERVICE_H   */
