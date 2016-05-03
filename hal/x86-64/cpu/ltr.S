/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Load Task State Segment region                                    */
/*                                                                    */
/**********************************************************************/

#define ASM_FILE   1
#include <hal/multiboot2.h>
#include <hal/arch-page.h>
#include <hal/pgtbl.h>
#include <hal/arch-cpu.h>	
#include <hal/segment.h>

.code64
/* void ltr(uint16_t sel);  */
.global ltr
ltr:
	ltr    %di
	retq
	
