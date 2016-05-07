/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  relevant definitions                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_MESSAGES_H)
#define  _KERN_MESSAGES_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/proc-service.h>
#include <kern/vm-service.h>
#include <kern/thread-service.h>

typedef struct _pri_string{
	int    req;
	int     rc;
	char  *msg;
	size_t len;
}pri_string_msg;

typedef struct _kname_service_msg{
	int          req;
	endpoint      id;
	const char *name;
	size_t       len;
	int           rc;
}kname_service_msg;

typedef union _msg_body{
	pri_string_msg sys_pri_dbg_msg;
	kname_service_msg    kname_msg;
	thr_service            thr_msg;
	proc_service          proc_msg;
	vm_service              vm_msg;
}msg_body;
#endif  /*  _KERN_MESSAGES_H   */
