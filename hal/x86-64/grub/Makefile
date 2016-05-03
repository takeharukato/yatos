# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=../..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include
subdirs=iso

all:

clean:
	for dir in ${subdirs} ; do 		\
		${MAKE} -C $${dir} clean;	\
	done

distclean:clean
	for dir in ${subdirs} ; do 		\
		${MAKE} -C $${dir} distclean;	\
	done
	${RM} *~

