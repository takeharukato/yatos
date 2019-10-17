/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  segment definitions                                               */
/*                                                                    */
/**********************************************************************/
#if !defined(__HAL_SEGMENT_H)
#define __HAL_SEGMENT_H

#define GDT_NULL1_SEL              0
#define GDT_NULL2_SEL              1
#define GDT_KERN_CODE32_SEL	   2
#define GDT_KERN_DATA32_SEL	   3
#define GDT_KERN_CODE64_SEL	   4
#define GDT_KERN_DATA64_SEL	   5
#define GDT_USER_CODE64_SEL	   6
#define GDT_USER_DATA64_SEL	   7
#define GDT_TSS64_SEL	           8

#define GDT_NULL1                0x0
#define GDT_NULL2                0x8
#define GDT_KERN_CODE32	        0x10
#define GDT_KERN_DATA32	        0x18
#define GDT_KERN_CODE64	        0x20
#define GDT_KERN_DATA64	        0x28
#define GDT_USER_CODE64	        0x33
#define GDT_USER_DATA64	        0x3b
#define GDT_TSS64	        0x40

#if defined(ASM_FILE)

#define GDT_NULL_ENTRY 	.quad	0x0

#define GDT_KERNEL	        0x90	
#define GDT_SEG_64	        0xA0
#define GDT_SEG_32	        0xC0
#define GDT_USER	        0xf0

#define GDT_CS		         0xb
#define GDT_DS		         0x3

#define SET_GDT_ENTRY(arch, mode, type, base, limit)	\
	.word   ((limit) & 0xffff);             \
	.word   ((base) & 0xffff);                      \
	.byte   (((base) >> 16) & 0xff);                \
	.byte   ((mode) | (type));			\
	.byte   ((arch) | (((limit) >> 16) & 0xf));     \
	.byte   (((base) >> 24) & 0xff)
#else

#define X86_IDT_INTR_TYPE_ATTR                        (0xe) /* 32/64 bit Interrupt(0xf=110b) */
#define X86_IDT_TRAP_TYPE_ATTR                        (0xf) /* 32/64 bit Trap(0xf=111b)      */
#define X86_DESC_DPL_KERNEL                           (0)
#define X86_DESC_DPL_USER                             (3)
#define X86_DESC_RDONLY                               (0)
#define X86_DESC_RDWR                                 (1)
#define X86_DESC_NONEXEC                              (0)
#define X86_DESC_EXEC                                 (1)
#define X86_DESC_32BIT_MODE                           (0)
#define X86_DESC_64BIT_MODE                           (1)
#define X86_DESC_64BIT_SEG                            (0)
#define X86_DESC_32BIT_SEG                            (1)
#define X86_DESC_BYTE_SIZE                            (0)
#define X86_DESC_PAGE_SIZE                            (1)

#define X86_64_SEGMENT_CPUINFO_OFFSET                 (1024)
#define X86_64_SEGMENT_CPUINFO_PAGE_ORDER             (2)
typedef struct _region_descriptor {
	uint16_t rd_limit:16;           /* segment extent */
	uint64_t rd_base:64 __attribute__((packed));   /* base address  */
} __attribute__((packed)) region_descriptor;

typedef struct _tss64 {
	uint32_t  resv1;
	uint64_t   rsp0;
	uint64_t   rsp1;
	uint64_t   rsp2;
	uint64_t  resv2;
	uint64_t   ist1;
	uint64_t   ist2;
	uint64_t   ist3;
	uint64_t   ist4;
	uint64_t   ist5;
	uint64_t   ist6;
	uint64_t   ist7;
	uint64_t  resv3;
	uint16_t  resv4;
	uint16_t  iomap;
} __attribute__((packed)) tss64;

typedef struct _gdt_descriptor{
	uint16_t     limit0;
	uint16_t      base0;
	uint8_t       base1;
	unsigned   access:1;
	unsigned       rw:1;
	unsigned       dc:1;
	unsigned     exec:1;
	unsigned    resv0:1;
	unsigned      dpl:2;
	unsigned  present:1;
	unsigned   limit1:4;
	unsigned      avl:1;
	unsigned     mode:1;
	unsigned     size:1;
	unsigned     gran:1;
	uint8_t       base2;
}__attribute__((packed)) gdt_descriptor;

typedef struct _idt_descriptor{
	uint16_t base0;
	uint16_t sel;
	unsigned ist:3;
	unsigned resv0:5;
	unsigned type:4;
	unsigned resv1:1;
	unsigned dpl:2;
	unsigned present:1;
	uint16_t base1;
	uint32_t base2;
	uint32_t resv2;
} __attribute__((packed)) idt_descriptor;

#define X86_64_IDT_INITIALIZER(_isr, _sel, _ist, _type, _dpl, _present) {	        \
	.base0 = (uint16_t)( ( (uint64_t)(_isr) ) & 0xffff),		                \
	.base1 = (uint16_t)( ( ( ( (uint64_t)(_isr) ) & 0xffff0000) >> 16 ) & 0xffff ), \
	.base2 = (uint32_t)( ( ( (uint64_t)(_isr) )  >> 32 ) & 0xffffffff ),            \
	.sel = (_sel),                                                                  \
	.ist = (_ist),                                                                  \
	.type = (_type),                                                                \
	.dpl  = (_dpl),                                                                 \
	.present = 1,                                                                   \
	}
void init_segments(void *_local_page, tss64 **tssp);
void init_idt(idt_descriptor **_idtp);
void lgdtr(void *_gdtr, uint16_t _code_seg, uint16_t _data_seg);
void lidtr(void *_idtr);
void load_interrupt_descriptors(void *_p, size_t _size);
void ltr(uint16_t seg);

#endif  /*  !ASM_FILE  */

#endif  /*  __HAL_SEGMENT_H  */
