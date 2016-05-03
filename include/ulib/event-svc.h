/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  userland event relevant definitions                               */
/*                                                                    */
/**********************************************************************/
#if !defined(_ULIB_EVHANDLER_H)
#define  _ULIB_EVHANDLER_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/async-event.h>

#include <ulib/yatos-ulib.h>

void _yatos_register_common_event_handler(void);
void yatos_event_return(event_frame *_evf);
#endif  /*  _ULIB_EVHANDLER_H   */
