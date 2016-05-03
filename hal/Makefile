# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include
subdirs=hal
cleandirs=${subdirs}
objects=


all: hal
	for dir in ${subdirs} ; do \
	${MAKE} -C $${dir} all ;\
	done

hal:
	${RM} hal
	${LN} -sf ${CPU} hal

clean-kern: hal
	${MAKE} -C hal $@

clean:hal
	for dir in ${cleandirs} ; do \
	${MAKE} -C $${dir} clean ;\
	done
	${RM} *.o ${lib}

distclean:hal clean
	for dir in ${cleandirs} ; do \
	${MAKE} -C $${dir} distclean ;\
	done
	${RM} *~
	${RM} hal

