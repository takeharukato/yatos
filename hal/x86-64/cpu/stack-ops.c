/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x86-64 backtrace routines                                         */
/*                                                                    */
/**********************************************************************/
#include <stdint.h>
#include <stddef.h>

#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/backtrace.h>

#include <hal/kernlayout.h>

/** バックトレース情報を探査する
    @param[in] _trace_out  バックトレース出力関数へのポインタ
    @param[in,out]  basep  ベースポインタのアドレス
    @param[in,out]  argp   呼び出し元から引き渡されるプライベートデータ
 */
void 
hal_back_trace(int (*_trace_out)(int _depth, uintptr_t *_bpref, void *_caller, void *_next_bp, 
					   void *_argp), void *basep, void *argp) {
	uintptr_t bp;
	uintptr_t *bpref;
	int        depth;
	int           rc;

	if ( _trace_out == NULL )
		return;

	if ( basep == NULL )
		get_base_pointer(&bp);
	else
		bp = *(uintptr_t *)basep;

	for(depth = 0, bpref = (uintptr_t *)bp; 
	    (bpref != NULL) 
		    && ( (uintptr_t)bpref >= KERN_VMA_BASE);
	    bpref = (uintptr_t *)bpref[0], ++depth) {

		rc = _trace_out(depth, bpref, (void *)bpref[1], (void *)bpref[0], argp);
		if ( rc < 0 )
			break;
	}
}

