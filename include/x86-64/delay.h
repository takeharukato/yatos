/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  milli-second and micro-second delay definitions                   */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_DELAY_H)
#define  _HAL_DELAY_H 
#include <stdint.h>

void mdelay(uint64_t ms);
void udelay(uint64_t us);
#endif  /*  _HAL_DELAY_H   */
