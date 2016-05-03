/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  process system call relevant definitions                          */
/*                                                                    */
/**********************************************************************/
#if !defined(_ULIB_VM_SVC_H)
#define  _ULIB_VM_SVC_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <ulib/yatos-ulib.h>

void *yatos_vm_sbrk(intptr_t increment);

#endif  /*  _ULIB_VM_SVC_H   */
