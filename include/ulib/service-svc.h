/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  service relevant definitions                                      */
/*                                                                    */
/**********************************************************************/
#if !defined(_ULIB_SERVICE_SVC_H)
#define  _ULIB_SERVICE_SVC_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <ulib/yatos-ulib.h>

int yatos_register_service(const char *_name);
int yatos_unregister_service(const char *_name);
int yatos_lookup_service(const char *_name, endpoint *_ep);
#endif  /*  _ULIB_SERVICE_SVC_H   */
