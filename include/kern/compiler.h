/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Compiler relevant definitions                                     */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_COMPILER_H)
#define  _KERN_COMPILER_H 

#if !defined(ASM_FILE)

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Fall through */
#if defined(__cplusplus) && __cplusplus >= 201703L
#define __FALLTHROUGH [[fallthrough]]
#elif defined(__cplusplus) && defined(__clang__)
#define __FALLTHROUGH [[clang::fallthrough]]
#elif __GNUC__ >= 7
#define __FALLTHROUGH __attribute__((__fallthrough__))
#else    /*   __GNUC__ >= 7  */
#define __FALLTHROUGH do {} while (0)
#endif  /*   __cplusplus && __cplusplus >= 201703L  */

#endif  /*  !ASM_FILE  */

#endif  /*  _KERN_COMPILER_H   */
