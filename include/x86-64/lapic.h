/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Local APIC definitions                                            */
/*                                                                    */
/**********************************************************************/
#if !defined(__HAL_LAPIC_H)
#define __HAL_LAPIC_H
#define APIC_REGISTER_APICID          (0x20)
#define APIC_REGISTER_APICVER         (0x30)
#define	APIC_REGISTER_TASKPRIOR       (0x80)
#define	APIC_REGISTER_EOI             (0xb0)
#define	APIC_REGISTER_LDR             (0xd0)
#define	APIC_REGISTER_DFR             (0xe0)
#define APIC_REGISTER_SPURIOUS        (0xf0)
#define APIC_REGISTER_ESR            (0x280)
#define APIC_REGISTER_ICRL           (0x300)
#define APIC_REGISTER_ICRH           (0x310)
#define APIC_REGISTER_LVT_TIMER      (0x320)
#define APIC_REGISTER_LVT_PERF       (0x340)
#define APIC_REGISTER_LVT_LINT0      (0x350)
#define APIC_REGISTER_LVT_LINT1      (0x360)
#define APIC_REGISTER_LVT_ERR        (0x370)

#define APIC_REGISTER_TMR_INITCNT    (0x380)
#define APIC_REGISTER_TMR_CURRCNT    (0x390)
#define APIC_REGISTER_TMR_DIV        (0x3E0)

#define APIC_REGISTER_LAST           (0x38F)

#define APIC_ENABLE                  (0x100)
#define APIC_CPUFOCUS                (0x200)
#define APIC_NMI                     (0x400)
#define APIC_DISABLE               (0x10000)
#define APIC_TMR_PERIODIC          (0x20000)
#define APIC_MASKED           (APIC_DISABLE)

#define APIC_CMD_INIT           (0x00000500)
#define APIC_CMD_STARTUP        (0x00000600)
#define APIC_CMD_DELIVS         (0x00001000)
#define APIC_CMD_ASSERT         (0x00004000)
#define APIC_CMD_DEASSERT       (0x00000000)
#define APIC_CMD_LEVEL          (0x00008000)
#define APIC_CMD_BCAST          (0x00080000)
#define APIC_CMD_BUSY           (0x00001000)
#define APIC_CMD_FIXED          (0x00000000)

/* APIC ID  */
#define APIC_ID_APICID_SHIFT            (24)   /*  Local APIC ID         */

/* APIC Timer Divide Value */
#define APIC_TIMER_DIV_BY_2             (0x0)  /*  0000b: Divide by 2    */
#define APIC_TIMER_DIV_BY_4             (0x1)  /*  0001b: Divide by 4    */
#define APIC_TIMER_DIV_BY_8             (0x2)  /*  0010b: Divide by 8    */
#define APIC_TIMER_DIV_BY_16            (0x3)  /*  0011b: Divide by 16   */
#define APIC_TIMER_DIV_BY_32            (0x8)  /*  1000b: Divide by 32   */
#define APIC_TIMER_DIV_BY_64            (0x9)  /*  1001b: Divide by 64   */
#define APIC_TIMER_DIV_BY_128           (0xa)  /*  1010b: Divide by 128  */
#define APIC_TIMER_DIV_BY_1             (0xb)  /*  1011b: Divide by 1    */

#define APIC_DEFAULT_LAPIC_BASE         (0xfee00000)  /*  QEmu Local APIC Addr  */

#define T_IRQ0                           32 

#define IRQ_TIMER                         0
#define IRQ_SPURIOUS                     31
#define IRQ_ERROR                        19

void init_local_apic(void);
void calibrate_tsc(void);
#endif  /*  __HAL_LAPIC_H  */
