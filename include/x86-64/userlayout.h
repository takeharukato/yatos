/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  user address space layout definitions                             */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_USERLAYOUT_H)
#define  _HAL_USERLAYOUT_H 

#define USER_TEXT_TOP       (0x400000)
#define USER_STACK_BOTTOM   (0x400000000000)  /*< 64TiB以前を使用. */
#define USER_VADDR_LIMIT    (0x800000000000)  /*< x86-64 正規形アドレスのユーザ側最大値  */

#endif  /*  _HAL_USERLAYOUT_H   */
