/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  relevant definitions                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_ULIB_UTILS_H)
#define  _ULIB_UTILS_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#include <ulib/yatos-ulib.h>

#define YATOS_PRINTF_BUFSIZ  (256)  /*< printfのバッファ長  */

int yatos_snprintf(char *buf, size_t size, const char *fmt,...);
int yatos_printf(const char *fmt,...);
int yatos_dbg_write(const char *buf, size_t len);
#endif  /*  _ULIB_UTILS_H   */
