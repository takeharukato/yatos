# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include
objects=event-mask.o event-alloc.o async-event.o
lib=libevent.a

all:${lib}

${lib}: clean-lib ${objects}
	${AR} ${ARFLAGS} $@ ${objects}
	${RANLIB} $@

clean: clean-lib
	${RM} *.o

distclean:clean
	${RM} *~

clean-lib:
	${RM} ${lib}
