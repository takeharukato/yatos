/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Low level datas                                                   */
/*                                                                    */
/**********************************************************************/

#define ASM_FILE   1
#include <hal/multiboot2.h>
#include <hal/arch-page.h>
#include <hal/segment.h>


.section .boot_data, "aw", @progbits

.global boot_entry_addr	
.global	pre_gdt_p
.global	pre_pml4
.global	pre_pdpt_low
.global	pre_pdpt_high
.global	pre_pd
.global	gdt
.global	idt
.global	idt_ptr

.extern  x86_64_prepare	
.extern	bsp_stack
boot_entry_addr:
	.quad x86_64_prepare

.balign 16
idt:
	#.fill 256, 0x10, 0x0  idt_t = 16(0x10)byte x 256 entries
	.fill 256, 0x10, 0x0
idt_ptr:
	.word	(. - idt - 1)
	.quad	idt


.balign 8
pre_gdt:
	GDT_NULL_ENTRY
	GDT_NULL_ENTRY
	SET_GDT_ENTRY( GDT_SEG_32, GDT_KERNEL, GDT_CS, 0x0, 0xFFFFF)
	SET_GDT_ENTRY( GDT_SEG_32, GDT_KERNEL, GDT_DS, 0x0, 0xFFFFF)
	SET_GDT_ENTRY( GDT_SEG_64, GDT_KERNEL, GDT_CS, 0x0, 0xFFFFF)
	SET_GDT_ENTRY( GDT_SEG_64, GDT_KERNEL, GDT_DS, 0x0, 0xFFFFF)
	SET_GDT_ENTRY( GDT_SEG_64, GDT_USER, GDT_CS, 0x0, 0xFFFFF)
	SET_GDT_ENTRY( GDT_SEG_64, GDT_USER, GDT_DS, 0x0, 0xFFFFF)
pre_gdt_end:

pre_gdt_p:
	.word	pre_gdt_end - pre_gdt + 1
	.quad	pre_gdt


.balign PAGE_SIZE
pre_pml4:
    .zero   PAGE_SIZE
.balign PAGE_SIZE
pre_pdpt_low:
    .zero   PAGE_SIZE
.balign PAGE_SIZE
pre_pdpt_high:
    .zero   PAGE_SIZE
.balign PAGE_SIZE
pre_pd:
    .zero   PAGE_SIZE
.balign PAGE_SIZE
gdt:
    .zero   PAGE_SIZE

