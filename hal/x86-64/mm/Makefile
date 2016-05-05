# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=../../..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include
objects=boot-kernel-map.o kernel-map.o page-pool.o pgtbl.o
lib=libhal-mm.a

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
