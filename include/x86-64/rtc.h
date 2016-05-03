/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  MC146818 Real Time Clock relevant definitions                     */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RTC_H)
#define  _HAL_RTC_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>

#define MC146818_RTC_SELECT_REG     (0x70)
#define MC146818_RTC_READ_REG       (0x71)
#define MC146818_RTC_REGA           (0xa)
#define MC146818_RTC_SHUTDOWN       (0xf)
#define MC146818_RTC_SHUTDOWN_CODE  (0xa)
#define MC146818_RTC_UPDATE_BIT     (7)

#endif  /*  _HAL_RTC_H   */
