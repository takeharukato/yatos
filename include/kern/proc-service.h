/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  process service relevant definitions                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_PROC_SERVICE_H)
#define  _KERN_PROC_SERVICE_H 

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

#define PROC_SERV_REQ_SNDEV     (0)
#define PROC_SERV_REQ_CRETHR    (1)
#define PROC_SERV_REQ_GETRUSAGE (2)
typedef enum _proc_serv_sendev_type{
	PROC_SERV_SNDEV_THR=0,    /*<  スレッド固有イベント  */
	PROC_SERV_SNDEV_PROC=1,   /*<  プロセス内共有イベント  */
	PROC_SERV_SNDEV_ALLTHR=2, /*<  プロセス内スレッドへの同報イベント  */
}proc_serv_sendev_type;
typedef struct _proc_sys_send_event{
	tid                   dest;
	proc_serv_sendev_type type;
	event_no                id;
	event_data            data;
}proc_sys_send_event;

typedef struct _proc_sys_create_thread{
	tid               id;
	thr_prio        prio;
	void          *start;
	void           *arg1;
	void           *arg2;
	void           *arg3;
	void             *sp;
}proc_sys_create_thread;

typedef struct _proc_sys_getrusage{
	tid                id;
	thread_resource	  res;
}proc_sys_getrusage;

typedef struct _proc_service{
	int    req;	
	int     rc;
	union _proc_service_calls{
		proc_sys_send_event     sndev;
		proc_sys_create_thread crethr;
		proc_sys_getrusage    getrusg;
	}proc_service_calls;
}proc_service;

void proc_service_init(void);
#endif  /*  _KERN_PROC_SERVICE_H   */
