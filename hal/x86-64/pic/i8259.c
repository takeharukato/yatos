/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Intel 8259 Programmable Interrupt Controller(PIC) routines        */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/errno.h>
#include <kern/spinlock.h>
#include <kern/irq.h>

#include <hal/portio.h>

#define I8259_MAX_IRQ_NR (16)  /* Intel i8259の割込み数  */

static int i8259_enable_irq(intr_no _no);
static int i8259_disable_irq(intr_no _no);
static int i8259_eoi_irq(intr_no _no);

static irq_cntlr i8259_ctlr = __IRQ_CTRL_INITIALIZER(i8259_enable_irq, i8259_disable_irq, i8259_eoi_irq);

/** Intel i8259で指定したIRQのi8259の割込みマスクを解除する
    @param[in] irq 割込み番号
    @retval 0   正常終了
 */
static int
i8259_enable_irq(intr_no no) {
	uint8_t mask;
	uint8_t irq;

	kassert( no < I8259_MAX_IRQ_NR );
	irq = no & ~((uint8_t)0);

	if (irq < 8)  { /*  マスター  */

		/* 現在のマスクから指定されたIRQのマスクを取り除いたマスク値を算出  */
		mask = in_port_byte(I8259_PIC1_BASE_ADDR2) & ~( 1 << irq );
		/* 算出したマスクを設定 */
		out_port_byte(I8259_PIC1_BASE_ADDR2, mask);
	} else {  /*  スレーブ  */

		/* 現在のマスクから指定されたIRQのマスクを取り除いたマスク値を算出  */
		mask = in_port_byte(I8259_PIC2_BASE_ADDR2) & ~( 1 << (irq - 8) );
		/* 算出したマスクを設定 */
		out_port_byte (I8259_PIC2_BASE_ADDR2, mask);
	}

	return 0;
}

/** Intel i8259で指定したIRQのi8259の割込みをマスクする
    @param[in] irq 割込み番号
    @retval 0   正常終了
 */
static int
i8259_disable_irq(intr_no no) {
	uint8_t mask;
	uint8_t irq;

	irq = no & ~((uint8_t)0);

	kassert( irq < I8259_MAX_IRQ_NR );

	if (irq < 8) { /*  マスター  */

		/* 現在のマスク値と指定されたマスクのORを取り更新後のマスクを算出  */
		mask = in_port_byte(I8259_PIC1_BASE_ADDR2) | ( 1 << irq ) ;
		/* 算出したマスクを設定 */
		out_port_byte(I8259_PIC1_BASE_ADDR2, mask);
	} else {  /*  スレーブ  */

		/* 現在のマスク値と指定されたマスクのORを取り更新後のマスクを算出  */
		mask = in_port_byte(I8259_PIC2_BASE_ADDR2) | ( 1 << (irq - 8)) ;
		/* 算出したマスクを設定 */
		out_port_byte(I8259_PIC2_BASE_ADDR2, mask);
	}

	return 0;
}

/** Intel i8259で指定したIRQのi8259の割込みに対するEnd of Interruptを発行する
    @param[in] irq 割込み番号
    @retval 0   正常終了
 */
static int
i8259_eoi_irq(intr_no no) {
	uint8_t irq;

	irq = no & ~((uint8_t)0);

	kassert( irq < I8259_MAX_IRQ_NR );

	i8259_disable_irq(no);  /*  ISRクリア中に割り込まないようマスクする  */

	/*
	 * ISRをクリアする
	 */
	if ( irq < 8 )   /* マスタ側にEOIを投げる  */
		out_port_byte(I8259_PIC1_OCW2_ADDR, pic_mk_spec_eoi(irq));
	else {
		/*
		 * カスケード接続されたi8259の場合, スレーブにEOIを投げた後で,
		 * マスターのカスケード接続IRQに対してEOIを投げる。
		 * スレーブの割込みは, マスターコントローラを介して, 
		 * CPUへ通知されるためマスター側で割込みをキューイングできる
		 * ようにマスタのカスケード接続IRQのキューをクリアするためである。
		 */

		/*  スレーブ側のコントローラにEOIを投げる  */
		out_port_byte(I8259_PIC2_OCW2_ADDR, pic_mk_spec_eoi(irq)); 
		/*  マスタ側のコントローラにEOIを投げる  */
		out_port_byte(I8259_PIC1_OCW2_ADDR, pic_mk_spec_eoi(I8259_PIC1_ICW3_CODE));
	}

	return 0;
	
}

/** 割込みマスクを設定する
    @param[in] msk 設定するマスク
 */
void
i8259_update_irq_mask(intr_mask_state msk) {
	uint16_t i8259_mask;

	i8259_mask = msk & ~( (uint16_t)0 );
	i8259_mask &= ~( 1 << I8259_PIC1_ICW3_CODE );  /*  カスケード接続は空ける  */

	out_port_byte(I8259_PIC1_BASE_ADDR2, ( i8259_mask & ~((uint8_t)0) ) );
	out_port_byte(I8259_PIC2_BASE_ADDR2, ( i8259_mask >> 8 ) );
}

/** i8259割込みコントローラの初期化 
    IRQ0以降を割り込みベクタ0x20以降に割込むようにコントローラを設定し,
    CPU例外との衝突が起こらないようにする。
    また, 初期状態として, PIC1とPIC2との接続箇所を除き割込みをマスク
    しておく。
 */
void
init_i8259_pic(void) {
	int i, rc;

	/* PIC 1*/
	out_port_byte(I8259_PIC1_BASE_ADDR1, I8259_PIC1_ICW1_CODE);
	out_port_byte(I8259_PIC1_BASE_ADDR2, I8259_PIC1_ICW2_CODE);
	out_port_byte(I8259_PIC1_BASE_ADDR2, I8259_PIC1_ICW3_CODE);
	out_port_byte(I8259_PIC1_BASE_ADDR2, I8259_PIC1_ICW4_CODE);

	/* PIC2  */
	out_port_byte(I8259_PIC2_BASE_ADDR1, I8259_PIC2_ICW1_CODE);
	out_port_byte(I8259_PIC2_BASE_ADDR2, I8259_PIC2_ICW2_CODE);
	out_port_byte(I8259_PIC2_BASE_ADDR2, I8259_PIC2_ICW3_CODE);
	out_port_byte(I8259_PIC2_BASE_ADDR2, I8259_PIC2_ICW4_CODE);

	/*
	 * 初期状態として割込みを全マスクする
	 */
	for(i = 0; I8259_MAX_IRQ_NR > i; ++i ) {

		if ( i == I8259_PIC1_ICW3_CODE )
			continue;
		rc = i8259_disable_irq((uint8_t)(i));
		kassert(rc == 0);
	}

	rc = i8259_enable_irq(I8259_PIC1_ICW3_CODE);  /* IRQ2をカスケード接続用に空ける  */
	kassert(rc == 0);

	for(i = 0; I8259_MAX_IRQ_NR > i; ++i )  /*  割込みコントローラを登録する  */
		kcom_irq_register_controller(i, &i8259_ctlr);

	return;
}
