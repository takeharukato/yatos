# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include
stdfuncs=${top}/klib/memset.o ${top}/klib/strlen.o ${top}/klib/strnlen.o \
	${top}/klib/doprintf.o ${top}/klib/memcpy.o
objects=bss.o errno.o thread-svc.o event-svc.o lpc-svc.o service-svc.o \
	uprintf.o proc-svc.o vm-svc.o event-handlers.o		 \
	event-mask.o				 \
	${stdfuncs}
crt_object=start.o
lib=libyatos.a

all: crt0.o ${lib}

crt0.o: clean-lib  ${crt_object}
	${AR} ${ARFLAGS} $@  ${crt_object}

${lib}: clean-lib ${objects}
	${AR} ${ARFLAGS} $@ ${objects}
	${RANLIB} $@

${top}/klib/memset.o:
	${MAKE} -C ${top}/klib memset.o

${top}/klib/memcpy.o:
	${MAKE} -C ${top}/klib memcpy.o

${top}/klib/strlen.o:
	${MAKE} -C ${top}/klib strlen.o

${top}/klib/doprintf.o:
	${MAKE} -C ${top}/klib doprintf.o

${top}/klib/strnlen.o:
	${MAKE} -C ${top}/klib strnlen.o

clean: clean-lib
	${RM} *.o 

distclean:clean
	${RM} *~

clean-lib:
	${RM} ${lib} crt0.o
