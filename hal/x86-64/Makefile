# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=../..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include $(shell echo ${CONFIG_HAL_OPT_FLAGS})
targets=kernel
subdirs=boot cpu dev exc mm pic thr timer
cleandirs=${subdirs} asm-offset 
hallibs=${top}/hal/hal/boot/libhal-boot.a 	\
	${top}/hal/hal/cpu/libhal-cpu.a 	\
	${top}/hal/hal/dev/libhal-dev.a 	\
	${top}/hal/hal/exc/libhal-exc.a	\
	${top}/hal/hal/mm/libhal-mm.a	\
	${top}/hal/hal/pic/libhal-pic.a	\
	${top}/hal/hal/thr/libhal-thr.a	\
	${top}/hal/hal/timer/libhal-timer.a

hal-config-boot-obj = ${top}/hal/hal/boot/boot.o ${top}/hal/hal/boot/data.o 

all: clean-kern ${targets}

${targets}: mk-hal-libs ${hal-config-boot-obj}
	${LD} ${LDFLAGS}  $(shell echo ${CONFIG_HAL_LDFLAGS}) 	\
		-nostdlib  -T kernel.lds  -o ${top}/kernel.elf 	\
		${hal-config-boot-obj} 				\
		--start-group ${kernlibs} ${hallibs} --end-group
	${CP} ${top}/kernel.elf ${top}/kernel-dbg.elf
	${OBJDUMP} -S ${top}/kernel-dbg.elf > ${top}/kernel.asm
	${NM} -a -n ${top}/kernel-dbg.elf > ${top}/kernel.sym
	${STRIP} -g ${top}/kernel.elf


gen-asm-offset: 
	${MAKE} -C asm-offset 

${top}/${program_name}.iso: ${targets}
	mkdir -p ${top}/rfs/boot/grub
	mkdir -p ${top}/rfs/boot/modules
	cp ${top}/hal/x86-64/grub/iso/grub.cfg ${top}/rfs/boot/grub
	cp ${top}/kernel.elf ${top}/rfs/boot
	cp ${top}/kernel-dbg.elf  ${top}/rfs/boot
	cp ${top}/user/*.elf  ${top}/rfs/boot
	${GRUB2_RESCUE} -o ${top}/${program_name}.iso ${top}/rfs
	if [ -d ${top}/rfs ]; then     					\
	  	rm -fr ${top}/rfs;					\
	fi;

clean-kern: 
	${RM} ${top}/kernel.elf ${top}/kernel-dbg.elf ${top}/kernel.asm ${top}/kernel.syms

run: ${top}/${program_name}.iso
	${QEMU} ${QEMU_OPT} -cdrom $<

run-debug: ${top}/${program_name}.iso
	${QEMU} ${QEMU_OPT} ${QEMU_DBG_OPT} -cdrom $<

mk-hal-libs: 
	for dir in ${subdirs} ; do \
	${MAKE} -C $${dir} ;\
	done
clean:
	${RM} *.o ${lib} 
	for dir in ${cleandirs} ; do \
	${MAKE} -C $${dir} clean ;\
	done
	${RM} *.o ${targets} *.tmp *.elf asm-offset.s
	${RM} -f ${top}/include/hal/asm-offset.h

distclean:clean
	for dir in ${cleandirs} ; do \
	${MAKE} -C $${dir} distclean ;\
	done
	${RM} *~



