/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  architecture dependent low level kernel console definitions       */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_KCONSOLE_H)
#define  _HAL_KCONSOLE_H 
#include <kern/config.h>

void kconsole_putc(int _ch);
void init_kconsole(void);
#endif  /*  _HAL_KCONSOLE_H   */
