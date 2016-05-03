/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  relevant definitions                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_KERNEL_H)
#define  _KERN_KERNEL_H 

#if !defined(__IN_ASM_OFFSET)
#include <hal/asm-offset.h>
#endif  /*  __IN_ASM_OFFSET  */

#if !defined(ASM_FILE)
void kcom_start_kernel(void);
void dbg_console_service_init(void);
void system_threads_init(void);
#endif  /*  !ASM_FILE  */

#endif  /*  _KERN_KERNEL_H   */
