# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=../../..
include ${top}/Makefile.inc
ASM_OFFSET_CFLAGS = $(shell echo ${CONFIG_OPT_FLAGS}|sed -e 's@-ggdb[0-9]*@@g') ${KERN_GENERIC_CFLAGS} -I${top}/include $(shell echo ${CONFIG_HAL_OPT_FLAGS})

all: gen-asm-offset

asm-offset.s: asm-offset.c
	${CC} ${ASM_OFFSET_CFLAGS} -S -o $@ $<

gen-asm-offset: asm-offset.s
	${top}/tools/asmoffset/gen-asm-offset.py \
	-i ${top}/hal/hal/asm-offset/asm-offset.s \
	-o ${top}/include/hal/asm-offset.h

clean:
	${RM} *.o asm-offset.s

distclean: clean
	${RM} *~
