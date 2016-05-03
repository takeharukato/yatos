# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=../../..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include
objects=halt.o idt.o lgdtr.o lidtr.o ltr.o segment.o stack-ops.o x86_64-cpu.o \
	x86_64-interrupt.o x86_64-rflags.o x86_64-spinlock.o x86_64-xchg.o	\
	x86_64-fpuregs.o

lib=libhal-cpu.a

all:${lib} ${boot_objects}

${lib}: clean-lib ${objects}
	${AR} ${ARFLAGS} $@ ${objects}
	${RANLIB} $@

clean: clean-lib
	${RM} *.o

distclean:clean
	${RM} *~

clean-lib:
	${RM} ${lib}
