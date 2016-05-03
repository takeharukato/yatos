# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include
subdirs=x86-64 kern proc thr tim ulib
cleandirs=${subdirs}
objects=

all: hal

hal:
	${RM} hal
	${LN} -sf ${CPU} hal

clean: hal
	for dir in ${cleandirs} ; do \
	${MAKE} -C $${dir} clean ;\
	done
	${RM} *.o ${lib}

distclean:clean
	for dir in ${cleandirs} ; do \
	${MAKE} -C $${dir} distclean ;\
	done
	${RM} *~
	${RM} hal

