# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include
objects=doprintf.o memchr.o memcmp.o memcpy.o memmove.o memset.o \
	printf.o strchr.o strcmp.o strcpy.o strlen.o strncpy.o \
	strnlen.o strrchr.o strstr.o strncat.o strncmp.o strdup.o vsnprintf.o \
	list.o queue.o
lib=libklib.a

all:hal ${lib}

${lib}: clean-lib ${objects}
	${AR} ${ARFLAGS} $@ ${objects}
	${RANLIB} $@

hal:
	${MAKE} -C ${top} $@

clean:
	${RM} *.o ${lib}

distclean:clean
	${RM} *~

clean-lib:
	${RM} ${lib}
