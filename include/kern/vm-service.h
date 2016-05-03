/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  process service relevant definitions                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_VM_SERVICE_H)
#define  _KERN_VM_SERVICE_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>

#define VM_SERV_REQ_SBRK (0)

typedef struct _vm_sys_sbrk{
	intptr_t         inc;
	void   *old_heap_end;
}vm_sys_sbrk;

typedef struct _vm_service{
	int    req;	
	int     rc;
	union _vm_service_calls{
		vm_sys_sbrk sbrk;	
	}vm_service_calls;
}vm_service;

void vm_service_init(void);
#endif  /*  _KERN_VM_SERVICE_H   */
