/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  process service relevant definitions                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_THR_SERVICE_H)
#define  _KERN_THR_SERVICE_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>
#include <kern/thread-resource.h>
#include <kern/async-event.h>

#define THR_SERV_REQ_GET_EVMSK (0)
#define THR_SERV_REQ_SET_EVMSK (1)

typedef struct _thr_sys_mask_op{
	event_mask      mask;
}thr_sys_mask_op;


typedef struct _thr_service{
	int    req;	
	int     rc;
	union _thr_service_calls{
		thr_sys_mask_op maskop;
	}thr_service_calls;
}thr_service;

void thr_service_init(void);
#endif  /*  _KERN_THR_SERVICE_H   */
