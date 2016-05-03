/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Load interrupt descriptor table region                            */
/*                                                                    */
/**********************************************************************/

#define ASM_FILE   1
#include <hal/segment.h>

.code64
/* void lidtr(struct region_descriptor *rdp);  */
.global lidtr
lidtr:
	lidt    (%rdi)
	retq
	
