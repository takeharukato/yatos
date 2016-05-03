# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=.
targets=kernel
include ${top}/Makefile.inc
subdirs=kern page thr proc vm irq tim lpc event klib lib user tests
cleandirs=include hal ${subdirs} tools configs
mconf=tools/kconfig/mconf
archive_name=${program_name}-${program_version}

all: clean-kern ${targets}

${mconf}:
	${MAKE} -C tools

gtags:
	${GTAGS} -v

menuconfig: hal configs/Config.in ${mconf}
	${RM} include/kern/autoconf.h
	${mconf} configs/Config.in || :

include/kern/autoconf.h: .config
	${RM} -f $@
	${top}/tools/kconfig/conf-header.sh .config > $@

include/hal/asm-offset.h: hal include/kern/autoconf.h
	${MAKE} -C hal/hal gen-asm-offset

${targets}:include/kern/autoconf.h hal include/hal/asm-offset.h subsystem
	${MAKE} -C hal/hal $@

hal:
	${MAKE} -C include hal
	${MAKE} -C hal hal

subsystem: hal
	for dir in ${subdirs} ; do \
	${MAKE} -C $${dir} ;\
	done

run: hal all
	${MAKE} -C hal/hal $@

run-debug: hal all
	${MAKE} -C hal/hal $@

clean-kern:
	${MAKE} -C hal $@

clean:
	for dir in ${cleandirs} ; do \
	${MAKE} -C $${dir} clean ;\
	done
	${RM} *.o ${targets} *.tmp *.elf *.iso ..config.tmp *.asm *.sym \
		${top}/include/hal/asm-offset.h	                        \
		boot.log build.log


distclean:clean
	for dir in ${cleandirs} ; do \
	${MAKE} -C $${dir} distclean ;\
	done
	${RM}  *~ .config* _config include/kern/autoconf.h *.log \
		GPATH GRTAGS GSYMS GTAGS

dist:
	${RM} ${archive_name}.tar.gz
	${GIT} archive --format=tar --prefix=${archive_name}/ HEAD | gzip > ${archive_name}.tar.gz
