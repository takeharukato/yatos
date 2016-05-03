# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include
subdirs=
cleandirs=${subdirs}
objects=timer-handler.o timer.o
lib=libtim.a

all:${lib}

${lib}: clean-lib ${objects}
	${AR} ${ARFLAGS} $@ ${objects}
	${RANLIB} $@

clean: clean-lib
	${RM} *.o

distclean: clean
	for dir in ${cleandirs} ; do \
	${MAKE} -C $${dir} distclean ;\
	done
	${RM} *~

clean-lib:
	${RM} ${lib}
