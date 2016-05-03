/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Intel 8254  Programmable Interval Timer(PIT) relevant definitions */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_I8254_H)
#define  _HAL_I8254_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>

#define I8254_INPFREQ        (1193182UL)  /*<  input clock 1193180 Hz */
#define I8254_PORT_CHANNEL0  (0x40)  /*< I/O port for timer channel 0 */
#define I8254_PORT_CHANNEL1  (0x41)  /*< I/O port for timer channel 1 */
#define I8254_PORT_CHANNEL2  (0x42)  /*< I/O port for timer channel 2 */
#define I8254_PORT_MODECNTL  (0x43)  /*< I/O port for controling mode */
#define I8254_CMD_INTERVAL_TIMER (0x36) /*< Mode3(0x6 Square Wave Generator, 0x30 16 bit counter */
#endif  /*  _HAL_I8254_H   */
