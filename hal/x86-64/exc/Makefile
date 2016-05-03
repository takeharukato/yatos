# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=../../..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include
objects=common-traps.o build-trap.o vector.o x86_64-svc.o x86_64-handle-event.o x86_64-trap-ctx.o
lib=libhal-exc.a

all:${lib} ${boot_objects}

${lib}: clean-lib ${objects}
	${AR} ${ARFLAGS} $@ ${objects}
	${RANLIB} $@

vector.S: ${top}/hal/hal/tools/gen-vec.py
	${RM} $@
	${top}/hal/hal/tools/gen-vec.py > $@

vector.o : vector.S

clean: clean-lib
	${RM} *.o

distclean:clean
	${RM} *~ vector.S

clean-lib:
	${RM} ${lib}
