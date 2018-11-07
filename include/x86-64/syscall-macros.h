/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Syscall macro relevant definitions                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_SYSCALL_MACROS_H)
#define  _HAL_SYSCALL_MACROS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>

typedef uint64_t syscall_arg_type;  /*< システムコール引数の型  */
typedef int64_t  syscall_res_type;  /*< システムコール結果の型  */

/** システムコールマクロ
    @note 以下のようにレジスタに引数情報を設定して
    int 0x90トラップを発行する

    システムコール番号      rax
    第1引数                 rdi
    第2引数                 rsi
    第3引数                 rdx
    第4引数                 r10
    第5引数(未使用)          r8
*/

#define syscall0( res, no)			\
	do{					\
	__asm__ __volatile__ ("int $0x90\n\t"	\
	    : "=a" (res)			\
	    : "a"(no)				\
	    :  "memory", "cc", "r11", "rcx");	\
	} while(0)


#define syscall1( res, no, arg1)		\
	do{					\
	__asm__ __volatile__ ("int $0x90\n\t"	\
	    : "=a" (res)			\
	    : "a"(no), "D"(arg1)		\
	    :  "memory", "cc", "r11", "rcx");	\
	} while(0)

#define syscall2( res, no, arg1, arg2)		\
	do{					\
	__asm__ __volatile__ ("int $0x90\n\t"	\
	    : "=a" (res)			\
	    : "a"(no), "D"(arg1), "S"(arg2)	\
	    :  "memory", "cc", "r11", "rcx");	\
	} while(0)

#define syscall3( res, no, arg1, arg2, arg3)	        \
	do{					        \
	__asm__ __volatile__ ("int $0x90\n\t"		\
	    : "=a" (res)				\
	    : "a"(no), "D"(arg1), "S"(arg2), "d"(arg3)	\
	    :  "memory", "cc", "r11", "rcx");	        \
	} while(0)

#define syscall4( res, no, arg1, arg2, arg3, arg4)			 \
	do{								 \
		register uint64_t r10 asm("r10") = (uint64_t)arg4;	 \
									 \
		__asm__ __volatile__ ("int $0x90\n\t"			 \
		    : "=a" (res)					 \
		    : "a"(no), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10) \
		    :  "memory", "cc", "r11", "rcx");			 \
	} while(0)

#define syscall5( res, no, arg1, arg2, arg3, arg4, arg5)		 \
	do{								 \
		register uint64_t r10 asm("r10") = (uint64_t)arg4;	 \
		register uint64_t  r8  asm("r8") = (uint64_t)arg5;	 \
									 \
		__asm__ __volatile__ ("int $0x90\n\t"			 \
		    : "=a" (res)					 \
		    : "a"(no), "D"(arg1), "S"(arg2), "d"(arg3),		 \
		    , "r"(r10), "r"(r8)					 \
		    :  "memory", "cc", "r11", "rcx");			 \
	} while(0)


#endif  /*  _HAL_SYSCALL_MACROS_H  */
