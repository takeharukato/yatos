# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include
objects=main.o spinlock.o id-bitmap.o elfldr.o svc.o kname-service.o dbg-console.o \
	system-threads.o proc-server.o vm-server.o backtrace.o mutex.o
lib=libkern.a

all:${lib}

${lib}: clean-lib ${objects}
	${AR} ${ARFLAGS} $@ ${objects}
	${RANLIB} $@

clean: clean-lib
	${RM} *.o

distclean:clean
	for dir in ${cleandirs} ; do \
	${MAKE} -C $${dir} distclean ;\
	done
	${RM} *~

clean-lib:
	${RM} ${lib}
