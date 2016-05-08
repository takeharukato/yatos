# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=..
include ${top}/Makefile.inc
CFLAGS = ${USER_CFLAGS} -I${top}/include 
ulib_objects=${top}/lib/crt0.o ${top}/lib/libyatos.a
user1_objects=main.o
targets=user1.elf
all:${targets}

user1.elf:${user1_objects} ${ulib_objects}
	${RM}  ${top}/user/user.elf
	${LD} ${LDFLAGS}  $(shell echo ${CONFIG_HAL_LDFLAGS}) -nostdlib -T user.lds -o ${top}/user/user1.elf ${top}/lib/crt0.o main.o -L${top}/lib -lyatos

${top}/lib/crt0.o:
	${MAKE} -C ${top}/lib crt0.o

${top}/lib/libyatos.a:
	${MAKE} -C ${top}/lib libyatos.a
clean: 
	${RM} *.o user1.elf

distclean:clean
	${RM} *~

