# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  OS kernel sample
#  Copyright 2014 Takeharu KATO
#
-include ${top}/.config
program_name=yatos
program_version=1.0
CPU ?= `uname -m|sed -e "s/amd64/x86-64/" -e "s/x86_64/x86-64/" -e "s/i?86/ia32/"`
QEMU_CPU ?= `uname -m|sed -e "s/amd64/x86-64/" -e "s/i?86/i386/"`
HOST_CC = gcc
CC := ${CROSS_COMPILE}${HOST_CC}
AR := ${CROSS_COMPILE}ar
RANLIB := ${CROSS_COMPILE}ranlib
STRIP := ${CROSS_COMPILE}strip
OBJCOPY := ${CROSS_COMPILE}objcopy
OBJDUMP := ${CROSS_COMPILE}objdump
NM := ${CROSS_COMPILE}nm
QEMU := qemu-system-${QEMU_CPU}
QEMU_OPT := -m 4096M
QEMU_GRAPHIC_OPT := -vga std -serial stdio
QEMU_NOX_OPT := -nographic -serial mon:stdio
QEMU_DBG_OPT := -S -gdb tcp::1234
GTAGS ?= gtags
GRUB2_RESCUE=grub2-mkrescue
CP=cp
RM=rm -f
LN=ln
ARFLAGS=rvc
GIT=git
KERN_GENERIC_CFLAGS := $(shell echo ${CONFIG_GENERIC_CFLAGS}) $(shell echo ${CONFIG_HAL_CFLAGS})
USER_GENERIC_CFLAGS := $(shell echo ${CONFIG_GENERIC_CFLAGS}) $(shell echo ${CONFIG_HAL_USER_CFLAGS})
CFLAGS += $(shell echo ${CONFIG_OPT_FLAGS}) ${KERN_GENERIC_CFLAGS}
USER_CFLAGS += $(shell echo ${CONFIG_OPT_FLAGS}) ${USER_GENERIC_CFLAGS}
kernlibs=${top}/kern/libkern.a \
	${top}/page/libpage.a \
	${top}/thr/libthr.a \
	${top}/proc/libproc.a \
	${top}/vm/libvm.a \
	${top}/irq/libirq.a \
	${top}/tim/libtim.a \
	${top}/lpc/liblpc.a \
	${top}/event/libevent.a \
	${top}/klib/libklib.a \
	${top}/tests/libtests.a ${kernlib-config-objs-y}

.PHONY: clean distclean hal tools menuconfig clean-kern dist run iso

.SUFFIXES: .o .c .S

.c.o:
	${CC} ${CFLAGS} -c -o $@ $<

.S.o:
	${CC} ${CFLAGS} -c -o $@ $<

.md.pdf:
	${PANDOC} $< -o $$ -V documentclass=ltjarticle --latex-engine=lualatex

