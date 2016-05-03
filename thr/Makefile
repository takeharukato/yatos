# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include
objects=thread-info.o thread-sync.o thread-que.o thread-reaper.o thread-idle.o thread-schedule.o thread.o
lib=libthr.a

all:${lib}

${lib}: clean-lib ${objects}
	${AR} ${ARFLAGS} $@  ${objects}
	${RANLIB} $@

clean: clean-lib
	${RM} *.o

distclean:clean
	${RM} *~

clean-lib:
	${RM} ${lib}
