/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  boot codes                                                        */
/*                                                                    */
/**********************************************************************/

#define ASM_FILE   1
#include <kern/param.h>
#include <kern/thread-info.h>

#include <hal/multiboot2.h>
#include <hal/arch-page.h>
#include <hal/pgtbl.h>
#include <hal/arch-cpu.h>	
#include <hal/segment.h>
#include <hal/asm-offset.h>

#define  MBH_MAGIC                              MULTIBOOT2_HEADER_MAGIC
#define  MBH_ARCH                               MULTIBOOT_ARCHITECTURE_I386
#define  MBH_LENGTH                             (mbhdr_end - mbhdr)
#define  MBH_CHECKSUM                           -(MBH_MAGIC + MBH_ARCH + MBH_LENGTH)

.section .boot_text, "ax"
.code32

.global mbhdr
.align MULTIBOOT_INFO_ALIGN
mbhdr:
		# Basic
		.long           MBH_MAGIC
		.long           MBH_ARCH
		.long           MBH_LENGTH
		.long           MBH_CHECKSUM

		# End tag
		.word           0,0
		.long           0x8
mbhdr_end:

.global start
.extern bsp_stack
.extern pre_gdt_p

start:
		cld
		cli
		# Store GRUB data
		movl    %eax, %edi
		movl    %ebx, %esi
		
		# Setup Stack
		movl    $(bsp_stack + KSTACK_SIZE - THREAD_INFO_SIZE), %esp
		# Set thread info magic, cpu number
		movl    $THR_THREAD_INFO_MAGIC, TI_MAGIC_OFFSET(%esp)
		movl    $0x0, TI_INTRCNT_OFFSET(%esp)
		movl    $0x0, TI_PREEMPT_OFFSET(%esp)
		movl    $0x0, TI_FLAGS_OFFSET(%esp)
		movl    $0x0, TI_ARCH_FLAGS_OFFSET(%esp)
		movl    $0x0, TI_THREAD_OFFSET(%esp)
		movl    $0x0, TI_CPU_OFFSET(%esp)
	
        	# Setup pre-GDT
		lgdt    pre_gdt_p
		ljmp    $GDT_KERN_CODE32, $1f
1:
        	# Setup data segment selectors
		mov     $GDT_KERN_DATA32, %eax
		mov     %ax, %ds
		mov     %ax, %es
		mov     %ax, %ss
		xorl    %eax, %eax
		mov     %ax, %fs
		mov     %ax, %gs

		# Init FPU
		fninit

.extern pre_pml4
.extern pre_pdpt_low
.extern pre_pdpt_high
.extern pre_pd

	        movl    $(pre_pdpt_low), %eax
	        orl     $PAGE_PRESENT, %eax
	        movl    %eax, pre_pml4

	        movl    $(pre_pdpt_high), %eax
	        orl     $PAGE_PRESENT, %eax
	        movl    %eax, pre_pml4 + (PGTBL_ENTRY_SIZE * PML4_INDEX(KERN_VMA_BASE))

	        movl    $(pre_pd), %eax
		orl     $PAGE_PRESENT, %eax
	        movl    %eax, pre_pdpt_low  + (PGTBL_ENTRY_SIZE * PDPT_INDEX(KERN_PHY_BASE))
	        movl    %eax, pre_pdpt_high + (PGTBL_ENTRY_SIZE * PDPT_INDEX(KERN_VMA_BASE))

	        xorl    %eax, %eax    # physical_address 0x0 ...
	        orl     $PAGE_2MB, %eax
	        orl     $PAGE_WRITABLE, %eax
	        orl     $PAGE_PRESENT, %eax
	        movl    %eax, pre_pd + (PGTBL_ENTRY_SIZE * PD_INDEX(KERN_PHY_BASE))
	        movl    %eax, pre_pd + (PGTBL_ENTRY_SIZE * PD_INDEX(KERN_VMA_BASE))

		# Setup long mode page table
	        movl $(pre_pml4), %eax
	        movl %eax, %cr3

		#enable PAE and PSE and OSFXSR and OSXMMEXCPT 
	        movl    %cr4, %eax              # read Control register 4
	        orl     $CR4_PAE, %eax          # enable PAE
	        orl     $CR4_PSE, %eax          # enable PSE
		orl	$CR4_OS_FXSR, %eax      # enable OSFXSR
		orl	$CR4_OS_XMMEXCEPT, %eax # enable OSXMMEXCPT 
	        movl    %eax, %cr4              # re-write

        	#enter long mode
	        movl    $EFER, %ecx
	        rdmsr   # read EFER
	        bts     $8, %eax # enable long mode 
	        wrmsr   # rewrite

	        #enable paging, disable FPU Emulation and enable monitor FPU
		movl    $CR0_FPU_EMULATION, %ecx 
		notl	%ecx
		movl    %cr0, %eax
		andl	%ecx, %eax
	        orl     $CR0_PAGING, %eax
	        orl     $CR0_MONITOR_FPU, %eax
	        movl    %eax, %cr0

	        ljmp    $GDT_KERN_CODE64, $.entry_long

.code64
.extern boot_entry_addr
.entry_long:
	        # Setup Stack (Higher)
	        movq    $(KERN_VMA_BASE), %rax
	        addq    %rax, %rsp

	        # Setup data segment selectors
	        mov     $GDT_KERN_DATA64, %eax
	        mov     %ax, %ds
	        mov     %ax, %es
	        mov     %ax, %ss

	        # Just in Case
	        pushq   %rsi
	        pushq   %rdi
	        movq    $(KERN_VMA_BASE), %rax
	        addq    %rax, %rsi
		movq	(boot_entry_addr), %rax
		call    *%rax
loop:
	        hlt
	        jmp loop
