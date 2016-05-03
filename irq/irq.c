/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Interrupt Ready Queue(IRQ) relevant routines                      */
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
#include <kern/queue.h>
#include <kern/list.h>
#include <kern/thread.h>
#include <kern/page.h>
#include <kern/irq.h>

static irq_manager irq_db;  /*<  割込み管理情報  */

/** 割込みハンドラが登録されていることを確認する
    @param[in] no      割込み番号
    @param[in] handler 割込みハンドラ
    @retval true 既に指定したハンドラが登録されている
    @note 割込みエントリロックの獲得は呼出元で実施する
 */
static bool
is_irq_handler_installed_nolock(intr_no no, ihandler_res (*handler)(intr_no _ino, 
	private_inf _data, void *ctx)) {
	bool                  rc;
	intrflags          flags;
	irq_entry           *ent;
	list                 *li;
	irq_handler      *hndlrp;

	kassert( no < NR_IRQS );

	ent = &irq_db.entries[no];
	kassert( spinlock_locked_by_self(&ent->lock) );

	hal_cpu_disable_interrupt(&flags);
	rc = false;
	for( li = queue_ref_top( &ent->hdlr_que );
	     li != (list *)&ent->hdlr_que;
	     li = li->next) {

		hndlrp = CONTAINER_OF(li, irq_handler, link);
		if ( hndlrp->handler == handler ) {
			
			rc = true;
			break;
		}
	}	
	hal_cpu_restore_interrupt(&flags);

	return rc;
}

/** 割込みハンドラを登録する
    @param[in] no      割込み番号
    @param[in] handler 割込みハンドラ
    @param[in] iflags  割込みハンドラフラグ
    @param[in] data    プライベートデータ
    @retval    0       正常に登録した
    @retval  -ENOMEM   メモリが不足している
    @retval  -EBUSY    既に同一のハンドラが同一のIRQに登録されている
    @retval  -EPERM    割込みの占有が行えなかったまたは直接関数呼び出し型のハンドラと
                       非関数呼び出し型のハンドラとで割込み線を共有させようとした
 */
int
irq_register_irq_handler(intr_no no, ihandler_res (*handler)(intr_no _ino, private_inf _data, void *ctx), ihandler_flags iflags, private_inf data) {
	int                   rc;
	intrflags          flags;
	irq_entry           *ent;
	irq_handler        *hdlr;

	kassert( no < NR_IRQS );

	/*
	 * 割込みハンドラ情報用のメモリを獲得し, 割込みハンドラ情報を設定する
	 */
	hdlr = kmalloc( sizeof(irq_handler), KMALLOC_NORMAL );
	if ( hdlr == NULL ) 
		return -ENOMEM;

	list_init( &hdlr->link );
	hdlr->iflags = iflags;
	hdlr->private = data;
	hdlr->handler = handler;

	ent = &irq_db.entries[no];

	spinlock_lock_disable_intr( &ent->lock, &flags);
	if ( is_irq_handler_installed_nolock(no, handler) ) {

		rc = -EBUSY;  /*  同一のハンドラが同一のIRQに登録済み  */
		goto error_out;
	}

	if ( ( !( iflags & IRQHDL_FLAG_EXCLUSIVE ) ) && 
	    ( ent->iflags & IRQHDL_FLAG_EXCLUSIVE ) ) {
		
		/*  多重割込み不可能なハンドラが登録されているIRQに共有割込みを
		 *  登録しようとした。
		 */
		rc = -EPERM;
		goto error_out;
	}

	/*
	 * IRQに登録されている割込みの属性を更新
	 */
	if ( iflags & IRQHDL_FLAG_EXCLUSIVE )
		ent->iflags |= IRQHDL_FLAG_EXCLUSIVE;

	queue_add(&ent->hdlr_que, &hdlr->link);  /*  ハンドラのキューに登録  */

	irq_unset_mask(no);  /* 割込みマスクを解除  */

	spinlock_unlock_restore_intr( &ent->lock, &flags);

	return 0;

error_out:		
	kfree( hdlr );
	spinlock_unlock_restore_intr( &ent->lock, &flags);


	return rc;
}

/** 割込みハンドラの登録を抹消する
    @param[in] no      割込み番号
    @param[in] handler 割込みハンドラ
    @retval    0       正常に登録を抹消した
    @retval  -ENOENT   引数で指定したハンドラが登録されていない
 */
int
irq_unregister_irq_handler(intr_no no, ihandler_res (*handler)(intr_no _ino, private_inf _data, void *ctx)) {
	int                   rc;
	intrflags          flags;
	irq_entry           *ent;
	irq_handler       *hdlrp;
	list       *li, *next_li;

	kassert( no < NR_IRQS );

	ent = &irq_db.entries[no];

	spinlock_lock_disable_intr( &ent->lock, &flags);

	rc = -ENOENT;

	for( li = queue_ref_top( &ent->hdlr_que );
	     li != (list *)&ent->hdlr_que;
		li = next_li) { /* 指定された割込みの割込みハンドラを順次探査 */

		next_li = li->next;  /* リンク削除時にリストが破壊されないように
				      次のノードを獲得 */

		hdlrp = CONTAINER_OF(li, irq_handler, link);
		if ( hdlrp->handler == handler ) {  /*  抹消対象のハンドラが見つかった */

			rc = 0;

			list_del( &hdlrp->link );  /* 登録を抹消  */
			kfree( hdlrp );  /*  ハンドラ情報のメモリを解放  */

			if ( queue_is_empty(&ent->hdlr_que) )
				irq_set_mask(no);  /* ハンドラ未登録の場合は割込みをマスク */
			break;
		}
	}

	spinlock_unlock_restore_intr( &ent->lock, &flags);

	return rc;
}

/** 割込み処理共通部
    @param[in] no  割込み番号
    @param[in] ctx 割込みコンテキスト
 */
void 
kcom_handle_irqs(intr_no no, void *ctx) {
	intrflags flags;
	int          rc;
	irq_entry *ent;
	list      *li;
	irq_handler *hndlrp;
	ihandler_res res;

	kassert( no < NR_IRQS );
	kassert( hal_cpu_interrupt_disabled() );

	ent = &irq_db.entries[no];

	kassert( ent->controller->disable != NULL );
	kassert( ent->controller->enable != NULL );
	kassert( ent->controller->send_eoi != NULL );

	hal_cpu_disable_interrupt(&flags);
	spinlock_lock( &ent->lock );

	ti_inc_intr();  /*  自CPUの割込み多重度をインクリメント  */

	if ( ent->controller == NULL ) { /* 割込みコントローラが登録されていない  */
                  
		kprintf(KERN_ERR, "Interrupt Controller has not been installed at IRQ:%d.\n",
		    no);
		goto unlock_out;
	}

	/*
	 * 割込み受付を割込みコントローラに通知
	 * 同一割込み要因での多重割込みを抑止するため, 処理中の割込み番号の
	 * 割込みをマスクしてから割込み受け付け通知を割込みコントローラに発行
	 */
	rc = ent->controller->disable(no);  /*  処理中の割込みをマスク     */
	kassert( rc == 0 );

	rc = ent->controller->send_eoi(no);  /*  割込み受け付け通知を発行  */
	kassert( rc == 0 );

	/*
	 * 順番にハンドラを起動
	 */
	for( li = queue_ref_top( &ent->hdlr_que );
	     li != (list *)&ent->hdlr_que;
	     li = li->next) {

		hndlrp = CONTAINER_OF(li, irq_handler, link);

		if ( !( ent->iflags & IRQHDL_FLAG_EXCLUSIVE ) )
			hal_cpu_enable_interrupt();  /*  多重割込みハンドラの起動時は割込み許可  */

		res = hndlrp->handler( no, hndlrp->private, ctx);  /* ハンドラ呼出し  */
		hal_cpu_restore_interrupt(&flags);  /*  CPUへの割込み状態を復元  */

		if ( res == IRQHDL_RES_HANDLED ) {
			
			/*  ハンドラ内で処理が完結する割込みだった場合は, 
			 *  同一要因での割込み受付けを再開
			 */
			rc = ent->controller->enable(no);
			kassert( rc == 0 );
			goto unlock_out;  /*  割込み処理完了  */
		}
	}

	/*
	 * 処理対象のハンドラがいなければ, 偽の割り込みとして扱う.
	 */
	++ent->spurious_cnt;
	if ( ent->spurious_cnt == SPURIOUS_INTR_THRESHOLD ) {
		
		/*
		 * 所定回数以上偽割込みが発生したら対象の割込みをマスクし
		 * 対象IRQからの割込みの受付けを停止する。
		 */
		kprintf(KERN_WAR, "Spurious Interrupt irq=%d count=%d, disabled.\n", 
		    no, ent->spurious_cnt);
		rc = ent->controller->disable(no);
		kassert( rc == 0 );

		spinlock_lock( &irq_db.lock );
		irq_db.spurious_mask |= ( 1 << no );
		spinlock_unlock( &irq_db.lock );
	} else {
		
		/*  規定回数に達するまでは割込みを再開  */
		rc = ent->controller->enable(no);
		kassert( rc == 0 );
	}

unlock_out:
	ti_dec_intr();   /*  自CPUの割込み多重度をデクリメント  */
	spinlock_unlock( &ent->lock );
	hal_cpu_restore_interrupt(&flags);		
}

/** 割込みコントローラを登録する
    @param[in] no         割込み番号
    @param[in] controller 割込みコントローラ構造体
    @retval  0     正常登録できた
    @retval -EBUSY 多重登録を行おうとした
 */
int
kcom_irq_register_controller(intr_no no, irq_cntlr *controller) {
	int         rc;
	intrflags flags;
	irq_entry *ent;

	kassert( no < NR_IRQS );
	kassert( controller != NULL );

	ent = &irq_db.entries[no];

	rc = 0;
	spinlock_lock_disable_intr( &ent->lock, &flags);
	if ( ent->controller != NULL )
		rc = -EBUSY;
	else
		ent->controller = controller;
	spinlock_unlock_restore_intr( &ent->lock, &flags);

	return rc;
}

/** 割込みコントローラの登録を抹消する
    @param[in] no         割込み番号
    @retval  0      正常に登録抹消できた
    @retval -ENOENT コントローラが登録されていないエントリから
		    登録を抹消しようとした
 */
int
kcom_irq_unregister_controller(intr_no no) {
	int         rc;
	intrflags flags;
	irq_entry *ent;

	kassert( no < NR_IRQS );

	ent = &irq_db.entries[no];

	rc = 0;
	spinlock_lock_disable_intr( &ent->lock, &flags);
	if ( ent->controller == NULL )
		rc = -ENOENT;
	else
		ent->controller = NULL;
	spinlock_unlock_restore_intr( &ent->lock, &flags);

	return rc;
}

/** 割込みをマスクする
    @param[in] no 指定した割込み番号をマスクする
 */
void
irq_set_mask(intr_no no) {
	intrflags          flags;
	intr_mask_state new_mask;

	kassert( no < NR_IRQS );

	spinlock_lock_disable_intr( &irq_db.lock, &flags);	

	new_mask = irq_db.mask;
	new_mask |= ( ( (intr_mask_state)1 ) << no );
	hal_pic_update_irq_mask( new_mask | irq_db.spurious_mask );
	irq_db.mask = new_mask;
	spinlock_unlock_restore_intr( &irq_db.lock, &flags);
}

/** 割込みマスクを解除する
    @param[in] no 指定した割込み番号のマスクを解除する
 */
void
irq_unset_mask(intr_no no) {
	intrflags          flags;
	intr_mask_state new_mask;

	kassert( no < NR_IRQS );

	spinlock_lock_disable_intr( &irq_db.lock, &flags);	

	new_mask = irq_db.mask;
	new_mask &= ~( ( (intr_mask_state)1 ) << no );

	hal_pic_update_irq_mask( new_mask | irq_db.spurious_mask );
	irq_db.mask = new_mask;

	spinlock_unlock_restore_intr( &irq_db.lock, &flags);
}

/** 割込み管理の初期化
 */
void
irq_init_subsys(void) {
	int             i;
	irq_entry    *ent;

	memset(&irq_db, 0, sizeof(irq_manager));
	
	spinlock_init( &irq_db.lock );
	irq_db.mask = ~( (intr_mask_state )(0) );
	irq_db.spurious_mask = 0;
	for( i = 0; NR_IRQS > i; ++i) {

		ent = &irq_db.entries[i];

		spinlock_init( &ent->lock );
		queue_init( &ent->hdlr_que );
		ent->iflags = IRQHDL_FLAG_MULTI;
		ent->spurious_cnt = 0;
		ent->controller = NULL;
	}
}
