/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  IRQ manager relevant definitions                                  */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_IRQ_H)
#define  _KERN_IRQ_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>
#include <kern/queue.h>
#include <kern/list.h>
#include <kern/irq-ctrl.h>

#include <hal/traps.h>

/** 割込みハンドラフラグ
 */
#define  IRQHDL_FLAG_MULTI      (0x0)     /*< 多重割込みを許可する              */
#define  IRQHDL_FLAG_EXCLUSIVE  (0x1)     /*< 多重割込みを禁止する              */

/** ハンドラ終了コード
 */
typedef enum  _ihandler_res{
	IRQHDL_RES_HANDLED=0,    /*< 処理完了                          */
	IRQHDL_RES_NEXT=1        /*< 他のハンドラに処理を依頼          */
}ihandler_res;
/** 各割込みハンドラの管理情報
 */
typedef struct _irq_handler{
	list             link;   /*< 他のハンドラへのリンク */
	ihandler_flags iflags;   /*< 割込みフラグ           */
	private_inf   private;   /*< ハンドラ固有情報       */
	/*<  割込みハンドラ関数    */
	ihandler_res (*handler)(intr_no _ino, private_inf _data, void *_ctx);
}irq_handler;

/** 割込みハンドラ/コントローラ情報
 */
typedef struct _irq_entry{
	spinlock             lock;  /*< 割込みハンドラキューのロック  */
	ihandler_flags     iflags;  /*< 割込み共有状態                */
	obj_cnt_type spurious_cnt;  /*< 不正割込み検出回数            */
	queue            hdlr_que;  /*< 割込みハンドラのキュー        */
	irq_cntlr     *controller;  /*< 割込みコントローラ操作        */
}irq_entry;

/** 割込み管理情報
 */
typedef struct _irq_manager{
	spinlock                 lock;  /*< 割込み管理全体のロック  */
	intr_mask_state          mask;  /*< 割込みマスクの状態     */
	intr_mask_state spurious_mask;  /*< 偽割込みマスクの状態   */
	irq_entry    entries[NR_IRQS];  /*< 割込み操作情報         */
}irq_manager;

void irq_init_subsys(void);
void irq_set_mask(intr_no _no);
void irq_unset_mask(intr_no _no);
int irq_register_irq_handler(intr_no _no, 
    ihandler_res (*_handler)(intr_no , private_inf , void *), 
    ihandler_flags _iflags, private_inf _data);
int irq_unregister_irq_handler(intr_no _no, 
    ihandler_res (*_handler)(intr_no , private_inf , void *));

void kcom_handle_irqs(intr_no _no, void *_ctx);
int kcom_irq_register_controller(intr_no _no, irq_cntlr *_controller);
int kcom_irq_unregister_controller(intr_no _no);

void hal_pic_update_irq_mask(intr_mask_state _msk);
bool hal_is_intr_from_user(void *_ctx);
#endif  /*  _KERN_IRQ_H   */
