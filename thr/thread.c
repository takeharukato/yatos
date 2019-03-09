/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Thread operation routines                                         */
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
#include <kern/rbtree.h>
#include <kern/id-bitmap.h>
#include <kern/proc.h>
#include <kern/thread.h>
#include <kern/sched.h>
#include <kern/page.h>
#include <kern/cpu.h>
#include <kern/async-event.h>

#include <thr/thr-internal.h>

/** ツリー/キューとスレッドのロックを同時に取るときは, ツリー/キューのロックを取ってから
 *  スレッドのロックを取る  
 */

/** 全スレッド
 */
static thread_dic thr_created_tree = __THREAD_DIC_INITIALIZER(&thr_created_tree.booking);

/** 未使用スレッド  
*/
static thread_queue thr_free_queue = __TQ_INITIALIZER( &thr_free_queue.que ); 

/** 停止スレッド 
 */
static thread_queue thr_dormant_queue = __TQ_INITIALIZER( &thr_dormant_queue.que );

/** スレッドIDプール 
 */
static id_bitmap thr_idpool = __ID_BITMAP_INITIALIZER(ID_BITMAP_DEFAULT_RESV_IDS);

static int thread_id_cmp(struct _thread *_a, struct _thread *_b);

RB_GENERATE_STATIC(thread_id_tree, _thread, mnode, thread_id_cmp);

/** スレッドをTIDをキーに比較する
    @param[in] a 比較対象のスレッド1
    @param[in] b 比較対象のスレッド2
    @retval 0  両者のスレッドIDが等しい
    @retval 負 スレッド1のIDのほうが小さい
    @retval 正 スレッド1のIDのほうが大きい
 */
static int 
thread_id_cmp(struct _thread *a, struct _thread *b) {

	kassert( (a != NULL) && (b != NULL) );

	if ( a->tid < b->tid )
		return -1;

	if ( a->tid > b->tid )
		return 1;

	return 0;
}

/** カーネルスタックを割当てる
    @param[in] thr 割当て対象スレッド
    @retval  0      正常に割当てた
    @retval -ENOMEM メモリ不足により割当てに失敗した
 */
static int
thr_alloc_kernel_stack(thread *thr) {
	int         rc;
	kstack_type sp;

	/* スタックを生成 */
	rc = alloc_buddy_pages( (void **)&sp, KSTACK_ORDER, KMALLOC_NORMAL );
	if ( rc != 0 )
		return rc;

	memset(sp, 0, KSTACK_SIZE);  /* スタックの内容をクリア */

	/*
	 * カーネルスタック情報をスレッド構造体に設定
	 */
	thr->ksp = sp;  /* 割り当てたスタックに設定  */
	thr->ti = (kstack_type)ti_kstack_to_tinfo(sp);  /* スレッド情報を設定  */
	thr->last_ksp = thr->ti; /* スタック最終位置を最下底位に設定  */

	return 0;
}

/** スレッド設定処理共通部
    @param[in] thr   対象のスレッド構造体
    @retval  0       正常に生成した
 */
static void
create_thread_common(thread *thr, int prio) {
	intrflags flags;

	kassert( thr != NULL );
	kassert( thr->prio < THR_MAX_PRIO );
	kassert( thr->type == THR_TYPE_NONE );
	kassert( thr->status == THR_TSTATE_FREE );

	spinlock_lock_disable_intr( &thr_free_queue.lock, &flags );
	list_del( &thr->link );  /*  未使用スレッドキューから外す  */
	spinlock_unlock_restore_intr( &thr_free_queue.lock, &flags );

	thr->prio = prio;  /*  スレッドの属性にprioを設定     */

	spinlock_lock_disable_intr( &thr_dormant_queue.lock, &flags );
	tq_add(&thr_dormant_queue, thr);  /* 停止キューに追加  */
	thr->status = THR_TSTATE_DORMANT;  /*  休止状態に遷移  */
	spinlock_unlock_restore_intr( &thr_dormant_queue.lock, &flags );

	return;
}

/** スレッドの消費資源情報を初期化する
    @param[in] thr_res 初期化対象の消費資源情報
 */
static void
thread_resource_init(thread_resource *thr_res) {

	kassert( thr_res != NULL );

	thr_res->gen                = 0;
	thr_res->sys_time           = 0;
	thr_res->user_time          = 0;
	thr_res->children_sys_time  = 0;
	thr_res->children_user_time = 0;
}

/** スレッドのパラメータを初期化する
    @param[in] thr 初期化対象スレッド
    @note アイドルスレッドの初期化でも使用するため外部リンケージ
 */
void 
_thr_init_kthread_params(thread *thr) {
	intrflags flags;

	kassert( thr != NULL);

	thr->tid = THR_INVALID_TID;        /*  tidを無効TIDに設定  */

	/*
	 * lock, mlink, plink, linkを初期化
	 */
	spinlock_init( &thr->lock );

	list_init( &thr->plink );
	list_init( &thr->link );
	list_init( &thr->parent_link );

        /*  子スレッド待ち合わせオブジェクト/子スレッドキューの初期化  */
	sync_init_object( &thr->children_wait, SYNC_WAKE_FLAG_ALL, THR_TSTATE_WAIT );
	sync_init_object( &thr->parent_wait, SYNC_WAKE_FLAG_ALL, THR_TSTATE_WAIT );
	queue_init ( &thr->children );
	queue_init ( &thr->exit_waiters );
	
	/*  スレッド消費資源の初期化  */
	thread_resource_init( &thr->resource );

	lpc_msg_queue_init( &thr->mque );  /*  メッセージキューを初期化  */

	hal_fpctx_init(&thr->fpctx);       /*  浮動小数点コンテキストの初期化  */

	ev_queue_init( &thr->evque );
	thr->p = hal_refer_kernel_proc();  /*  procをカーネル空間に設定  */	

	spinlock_lock_disable_intr( &thr->p->lock, &flags );
	queue_add( &thr->p->threads, &thr->plink );  /* カーネル空間にスレッドを追加  */
	spinlock_unlock_restore_intr( &thr->p->lock, &flags );

	/*
	 * タイムスライスのデフォルト値を設定
	 */
	thr->slice = THR_DEFAULT_SLICE;  
	thr->cur_slice = thr->slice; 
	kassert( thr->slice >= THR_NIN_SLICE );

	thr->exit_code = 0; 	/*  exit_codeを0に設定  */
}

/** カーネルスレッドを起動する
 */
void
kcom_launch_new_thread(int (*start)(void *), void *arg) {
	int rc;

	hal_cpu_enable_interrupt();
	rc = start(arg);       /* スレッド開始関数を呼び出す  */

	thr_exit(rc); 
	/* ここには来ない */
}

/** 全スレッドロックを保持していることを確認する
 */
bool
all_thread_locked_by_self(void) {

	return spinlock_locked_by_self( &thr_created_tree.lock );
}

/** 全スレッドロックを獲得する
    @param[in] flags 割込み状態保存領域のアドレス
 */
void
acquire_all_thread_lock(intrflags *flags) {

	spinlock_lock_disable_intr( &thr_created_tree.lock, flags );
}

/** 全スレッドロックを解放する
    @param[in] flags 割込み状態保存領域のアドレス
 */
void
release_all_thread_lock(intrflags *flags) {

	spinlock_unlock_restore_intr( &thr_created_tree.lock, flags );	
}

/** 生成済みのスレッドをTIDをキーに検索する
    @param[in] tid 検索キーとなるスレッドID
    @return NULLでないポインタ 見つかったスレッドのスレッド構造体へのポインタ
    @return NULL               keyで指定したスレッドが見つからなかった
 */
thread *
thr_find_thread_by_tid_nolock(tid key) {
	//intrflags    flags;
	thread        *res;
	thread     key_thr;

	kassert( spinlock_locked_by_self(&thr_created_tree.lock) );

	if ( ( key == THR_IDLE_TID ) || ( key == THR_INVALID_TID ) )
		return NULL;  /*  不当ID  */

	key_thr.tid = key;

	res = RB_FIND(thread_id_tree, &thr_created_tree.booking, &key_thr);
	if ( ( res != NULL) && ( res->status == THR_TSTATE_EXIT ) )
		res = NULL;  /* 終了しようとしているスレッド  */

	return res;
}

/** スレッドを生成する
    @param[in] thrp  生成したスレッド構造体のアドレスを配置する先
    @retval  0       正常に生成した
    @retval -ENOMEM  メモリ不足で生成に失敗した
 */
int
thr_new_thread(thread **thrp) {
	int          rc;
	intrflags flags;
	thread     *thr;

	kassert( thrp != NULL);

	/*  スレッド構造体を生成する  */
	thr = kmalloc( sizeof(thread), KMALLOC_NORMAL);
	if ( thr == NULL )
		return -ENOMEM;

	memset( thr, 0, sizeof( thread ) );
	
	_thr_init_kthread_params(thr);  /*  パラメタを初期化する  */

	rc = thr_alloc_kernel_stack(thr); /* スタックを生成する(スレッド構造体を引き渡す)  */
	if ( rc != 0 )
		goto free_thread_out;
	
	_ti_set_ti_with_thread(thr);  	/*  スレッド情報を初期化する  */

	thr->type = THR_TYPE_NONE;     /* 無属性スレッドに設定  */
	thr->status = THR_TSTATE_FREE;  /*  スレッドの状態をTHR_TSTATE_FREEに遷移  */

	spinlock_lock_disable_intr( &thr_free_queue.lock, &flags );
	tq_add( &thr_free_queue, thr);  /*  未使用スレッドキューに追加  */
	spinlock_unlock_restore_intr( &thr_free_queue.lock, &flags );

	*thrp = thr;  /*  生成したスレッドを返却する  */

	return 0;

free_thread_out:
	kfree( thr );
	return rc;
}

/** スレッドをカーネルスレッドとして利用する
    @param[in] thr   対象のスレッド構造体
    @param[in] prio  スレッドの優先度
    @param[in] thr_flags スレッドの属性情報
    @param[in] newid 設定するスレッドID
    @param[in] fn    スレッド開始関数
    @param[in] arg
    @retval  0       正常に生成した
    @retval -ENOMEM  メモリ不足で生成に失敗した
 */
int
thr_create_kthread(thread *thr, int prio, thread_flags thr_flags, tid newid, 
    int (*fn)(void *), void *arg) {
	int          rc;
	thread     *res;
	intrflags flags;

	kassert( thr != NULL );
	kassert( thr->prio < THR_MAX_PRIO );
	kassert( fn != NULL );
	kassert( (uintptr_t)fn >= KERN_VMA_BASE );

	/* スレッド作成処理の共通処理を実施  */
	create_thread_common(thr, prio);

	/*  カーネルスレッド用にIDを取得  */
	if ( newid == THR_INVALID_TID )
		rc = idbmap_get_id(&thr_idpool, ID_BITMAP_SYSTEM, &thr->tid);
	else {
	
		rc = idbmap_get_specified_id(&thr_idpool, newid, ID_BITMAP_SYSTEM);
		thr->tid = newid;
	}
	kassert( rc == 0 );

	thr->thr_flags = thr_flags;   /*  属性情報を設定  */

	/*  全スレッド追跡用のキューに追加  */	
	acquire_all_thread_lock( &flags );

	res = RB_INSERT(thread_id_tree, &thr_created_tree.booking, thr);
	kassert( res == NULL );

	release_all_thread_lock( &flags );  /*  全スレッド追跡用のキューに追加  */

	thr->type = THR_TYPE_KERNEL;  /*  スレッド種別をカーネルスレッドに設定  */

	/*  念のためKILL以外のイベントマスクをすべて閉じる  */
	ev_mask_fill( &thr->evque.masks );

	/*  スレッド開始関数, 起動時のスタックフレームを設定  */	
	hal_setup_kthread_function(thr, fn, arg);

	return 0;
}

/** スレッドをユーザスレッドとして利用する
    @param[in] thr       対象のスレッド構造体
    @param[in] prio      ユーザスレッドの優先度
    @param[in] thr_flags スレッドの属性情報
    @param[in] p         スレッドが動作するプロセス
    @param[in] ustart    ユーザランドの開始アドレス
    @param[in] arg1      ユーザランドに引き渡す第1引数(argc)
    @param[in] arg2      ユーザランドに引き渡す第2引数(argv)
    @param[in] arg3      ユーザランドに引き渡す第3引数(environment)
    @param[in] ustack    ユーザランドの開始スタック
    @retval  0           正常に生成した
    @retval -ENOENT      プロセスが終了している
 */
int
thr_create_uthread(thread *thr, int prio, thread_flags thr_flags, proc *p, 
    void *ustart, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, void *ustack) {
	int          rc;
	thread     *res;
	intrflags flags;

	kassert( thr != NULL );
	kassert( thr->prio < THR_MAX_USER_PRIO );
	kassert( p != NULL );
	kassert( p != hal_refer_kernel_proc() );
	
	spinlock_lock_disable_intr( &p->lock, &flags );

	if ( p->status == PROC_PSTATE_EXIT ) {

		spinlock_unlock_restore_intr( &p->lock, &flags );
		return -ENOENT;  /*  終了済みプロセスにスレッドを追加しようとした  */
	}

	spinlock_unlock_restore_intr( &p->lock, &flags );

	/* スレッド作成処理の共通処理を実施  */
	create_thread_common(thr, prio);

	/*  ユーザスレッド用にIDを取得  */
	rc = idbmap_get_id(&thr_idpool, ID_BITMAP_USER, &thr->tid);
	kassert( rc == 0 );

	thr->thr_flags = thr_flags;   /*  属性情報を設定  */

	/*  全スレッド追跡用のキューに追加  */
	acquire_all_thread_lock( &flags );

	res = RB_INSERT(thread_id_tree, &thr_created_tree.booking, thr);
	kassert( res == NULL );

	release_all_thread_lock( &flags );

	thr->type = THR_TYPE_USER;  /*  スレッド種別をユーザスレッドに設定  */

	/*  スレッド起動時のスタックフレームを設定  */	
	hal_setup_uthread_kstack(ustart, arg1, arg2, arg3, ustack, 
	    thr->ksp, &thr->last_ksp);

	spinlock_lock_disable_intr( &thr->p->lock, &flags );
	list_del( &thr->plink );  /* プロセス空間からスレッドを除去  */
	spinlock_unlock_restore_intr( &thr->p->lock, &flags );

	spinlock_lock_disable_intr( &p->lock, &flags );
	queue_add( &p->threads, &thr->plink );  /* 新しいプロセス空間に追加  */
	spinlock_unlock_restore_intr( &p->lock, &flags );

	thr->p = p;  /*  指定されたプロセスのアドレス空間で動作  */

	return 0;
}

/** スレッドを開始する
   @param[in] thr   開始するスレッド
   @param[in] ptid  親スレッドのスレッドID
   @retval  0       正常に開始
   @retval -EINVAL  スレッドが停止中でない
 */
int
thr_start(thread *thr, tid ptid) {
	intrflags flags;
	thread    *pthr;

	kassert( thr != NULL );

	if ( thr->status != THR_TSTATE_DORMANT )
		return -EINVAL;

	acquire_all_thread_lock( &flags );
	pthr = thr_find_thread_by_tid_nolock( ptid );
	/* 親スレッドがいない場合や親がカーネルスレッドの場合は, 待ち合わせフラグを落とす  */
	if ( pthr == NULL ) 
		thr->thr_flags &= ~THR_FLAG_JOINABLE; 
#if !defined(ENABLE_JOINABLE_KTHREAD)
	else if ( pthr->p == hal_refer_kernel_proc() ) 
		thr->thr_flags &= ~THR_FLAG_JOINABLE; 
#endif 
	else {

		/*
		 * 親スレッドの子スレッド管理キューに追加する
		 */
		spinlock_lock( &pthr->lock );
		queue_add( &pthr->children, &thr->parent_link );
		thr->ptid = ptid;  /* 親スレッドのIDを設定  */

		/*  親スレッドのイベントマスクをコピーする  */
		memcpy( &thr->evque.masks, &pthr->evque.masks, sizeof(event_mask) );

		spinlock_unlock( &pthr->lock );
	}
	release_all_thread_lock( &flags );
	
	spinlock_lock_disable_intr( &thr_dormant_queue.lock, &flags );

	tq_del(&thr_dormant_queue, thr);  /* 停止キューから削除  */

	spinlock_unlock_restore_intr( &thr_dormant_queue.lock, &flags );

	_sched_wakeup(thr);  /*  レディー状態に遷移してレディキューに追加  */

	if ( !ti_dispatch_disabled(current->ti) ) 
		sched_schedule();  /* スレッド開始に伴う再スケジュール  */

	return 0;
}

/** 自スレッドを終了する
    @param[in] rc    自スレッドの終了コード
    @retval  0       正常に生成した
    @retval -ENOMEM  メモリ不足で生成に失敗した
 */
void
thr_exit(exit_code rc) {
	intrflags flags;
	thread    *pthr;
	thread    *cthr;
	sync_reason res;
	list        *li;
	list      *next;

	kassert( current->status == THR_TSTATE_RUN );
	kassert( !check_recursive_locked(&current->lock) );
	kassert( list_not_linked(&current->link) );
	kassert( !ti_dispatch_disabled(current->ti) );


	current->exit_code = rc; /* 終了コードを設定  */
	/*
	 * 子スレッドの監視解除
	 */
	spinlock_lock_disable_intr( &current->lock, &flags );
	while( !queue_is_empty( &current->children) ) {

		cthr = CONTAINER_OF(queue_get_top( &current->children ),
		    thread, parent_link);
		list_del( &cthr->parent_link );
		cthr->thr_flags &= ~THR_FLAG_JOINABLE;
	}
	spinlock_unlock_restore_intr( &current->lock, &flags );

	/*
	 * 親スレッドとの終了同期
	 */
	acquire_all_thread_lock( &flags );
	pthr = thr_find_thread_by_tid_nolock( current->ptid );
	if ( pthr != NULL ) {

		/*
		 * 親スレッドの子スレッドキューから外す
		 */
		spinlock_lock( &pthr->lock );
		list_del( &current->parent_link );
		spinlock_unlock( &pthr->lock );
	} else {

		kassert( !( current->thr_flags & THR_FLAG_JOINABLE ) );
		current->thr_flags &= ~THR_FLAG_JOINABLE;
	}
	release_all_thread_lock( &flags );

	/* カーネルから起動したスレッドやJOINABLEでない
	 * スレッドは待ち合わせを行わない  
	 */
	if ( !( current->thr_flags & THR_FLAG_JOINABLE ) )
		goto enter_dead;

	/*
	 * 親スレッドに終了コード取得/資源情報回収を指示する
	 */
	spinlock_lock_disable_intr( &pthr->lock, &flags );

	kassert( list_not_linked( &current->parent_link ) );
	queue_add( &pthr->exit_waiters, &current->parent_link ); 
	sync_wake( &pthr->children_wait, SYNC_WAI_RELEASED );

	/*
	 * 親スレッドの終了コード/資源情報回収を待ち合わせる
	 */
	res=sync_wait(&current->parent_wait, &pthr->lock);

	spinlock_unlock_restore_intr( &pthr->lock, &flags );

	kassert( (res != SYNC_WAI_WAIT) && 
	    ( res != SYNC_WAI_TIMEOUT ) &&
	    ( res != SYNC_WAI_DELIVEV ) );
	kassert( !( current->thr_flags &THR_FLAG_JOINABLE ) );

enter_dead:
	current->status = THR_TSTATE_EXIT; /* スレッドを終了状態にする  */
	/*  リンク情報の更新中にプリエンプションによって,
	 *   レディーキューに戻らないようにする  
	 *   また, EXITにすることでイベントの配送を抑止する
	 */
	ti_disable_dispatch();

	/*  全スレッド追跡用のキューから削除  */
	acquire_all_thread_lock( &flags );
	RB_REMOVE(thread_id_tree, &thr_created_tree.booking, current); 
	release_all_thread_lock( &flags );

	spinlock_lock_disable_intr( &current->p->lock, &flags );
	list_del( &current->plink );  /* プロセス空間からスレッドを除去  */
	if ( current == current->p->master ) {

		if ( !queue_is_empty( &current->p->threads ) ) {

			/*
			 * 最終スレッドでなければ他にマスタースレッドを引き継ぐ
			 */
			current->p->master = 
				CONTAINER_OF(queue_ref_top( &current->p->threads ),
				    thread, plink);
		}
	}

	/* スレッドに保留されていたイベントを引き継ぐ, スレッド固有のイベントを破棄する */
	ev_handle_exit_thread_events();

	spinlock_unlock_restore_intr( &current->p->lock, &flags );

	spinlock_lock_disable_intr( &current->evque.lock, &flags );
        /*  自スレッドに未配送イベントは無い */
	kassert( ev_mask_empty( &current->evque.events ) );  
	spinlock_unlock_restore_intr( &current->evque.lock, &flags );

	lpc_destroy_msg_queue( &current->mque );    /* メッセージキューの削除  */

	proc_destroy(current->p); /*  プロセスの解放を試みる  */

        /*  プロセス空間が消失している可能性があるので
	 *  以降の処理は, カーネル空間で動作させる
	 */
	kassert( ti_dispatch_disabled( current->ti ) );
	current->p = hal_refer_kernel_proc();  

	/*
	 * 終了待ち子スレッドの待ちを解除
	 */
	spinlock_lock_disable_intr( &current->lock, &flags );

	for( li = queue_ref_top( &current->exit_waiters );
	     li != (list *)&current->exit_waiters;
	     li = next) {

		next = li->next;

		cthr = CONTAINER_OF(li, thread, parent_link);
		cthr->thr_flags &= ~THR_FLAG_JOINABLE;
		list_del( &cthr->parent_link );
		sync_wake( &cthr->parent_wait, SYNC_OBJ_DESTROYED);
	}

	spinlock_unlock_restore_intr( &current->lock, &flags );

	ti_enable_dispatch();  /*  親スレッドにCPUを渡すためディスパッチを許可する  */

	_thr_enter_dead();  /*  自スレッドの回収を依頼する */
	
	sched_schedule();  /*  自スレッド終了に伴うリスケジュール */

	/*  ここには来ない  */
}

/** 指定したスレッドを破棄する
    @param[in] thr 操作対象スレッド
    @retval 0 スレッドを正常に破棄した
    @retval -EMLINK レディキューやウエイトキューにつながっているスレッドを
                    破棄しようとした。
    @retval -EBUSY  終了していないスレッドを破棄しようとした
    @note 本関数はスレッド回収スレッドから呼ばれる。
    本関数は, 破棄対象のスレッドとは別のスレッドから本関数を呼び出す必要がある。
    ready状態への遷移を除いて, スレッドの状態遷移は自スレッドから行うことで,
    ロックの獲得
    自スレッド終了処理(thr_exit)の最後でスケジューラを呼び出し, 
    CPUを他のスレッドに明け渡す。
    スケジューラの呼出し時にはカーネルスタックが存在している必要があるため,
    カーネルスタックの解放は, 終了したスレッドと異なるスレッドが実施する。
 */
int
thr_destroy(thread *thr) {
	int           rc;
	intrflags  flags;

	kassert( !spinlock_locked_by_self(&thr_created_tree.lock) );
	kassert( !check_recursive_locked(&thr->lock) );
	kassert( thr != current );

	acquire_all_thread_lock( &flags );
	spinlock_lock( &thr->lock );

	if ( ( thr->status != THR_TSTATE_EXIT ) &&
	    ( thr->status != THR_TSTATE_DORMANT ) && 
	    ( thr->status != THR_TSTATE_FREE ) ) {

		rc = -EBUSY;  /*  未割り当て/停止/終了していないスレッドを破棄しようとした  */
		goto error_out;
	}

	/*
	 * 未使用状態/休止状態の場合の対処
	 */
	if ( thr->status == THR_TSTATE_DORMANT ) {

		/** THR_TSTATE_DORMANTの場合は, 生成済みスレッドのツリーに残存しているので削除
		 */
		RB_REMOVE(thread_id_tree, &thr_created_tree.booking, current); /*  全スレッド追跡用のキューから削除  */
	}

	if ( !list_not_linked( &thr->link ) ) {

		rc = -EMLINK;  /*  キューにつながっている  */
		goto error_out;
	}

	if ( thr_find_thread_by_tid_nolock(thr->tid) != NULL ) {
		
		rc = -EMLINK;  /*  全スレッド追跡用のツリーにつながっている  */
		goto error_out;
	}

	kassert( thr->tid != THR_IDLE_TID );

	idbmap_put_id( &thr_idpool, thr->tid );  /* IDを返却  */
	free_buddy_pages( thr->ksp );  /*  スタックを解放  */

	spinlock_unlock( &thr->lock );
	release_all_thread_lock( &flags );

	kfree( thr );  /*  スレッド情報を解放  */		

	return 0;

error_out:
	spinlock_unlock( &thr->lock );
	release_all_thread_lock( &flags );

	return rc;
}

/** 子スレッドの終了を待ち合わせる
    @param[in]  tid       待ち合わせ対象スレッドのスレッドID
    @param[in]  wflags    待ち合わせ対象スレッドの指定
    @param[out] exit_tidp 終了したスレッドのスレッドID格納アドレス
    @param[out] rcp       子スレッドの終了コード格納アドレス 
    @retval    0          待ち合わせ完了
    @retval   -EAGAIN     待ち中に割り込まれた
    @retval   -ENOENT     対象の子スレッドがいない
 */
int
thr_wait(tid wait_tid, thread_wait_flags wflags, tid *exit_tidp, exit_code *rcp){
	intrflags  flags;
	thread     *cthr;
	list         *li;
	list       *next;
	sync_reason  res;

	kassert( rcp != NULL );
	kassert( exit_tidp != NULL);

	spinlock_lock_disable_intr( &current->lock, &flags );
	while(1) {

		while( queue_is_empty(&current->exit_waiters ) )	{

			/* 子スレッドを待ち合わせる  */
			if ( wflags & THR_WAIT_NONBLOCK ) {

				spinlock_unlock_restore_intr( &current->lock, &flags );
				return -ENOENT;  /*  終了した子スレッドがない  */
			}
			res = sync_wait( &current->children_wait, &current->lock);
			if ( res != SYNC_WAI_RELEASED ) {

				spinlock_unlock_restore_intr( &current->lock, &flags );
				return -EAGAIN;
			}
			if ( res == SYNC_WAI_DELIVEV ) {

				spinlock_unlock_restore_intr( &current->lock, &flags );
				return -EINTR;
			}
		}

		for( cthr = NULL, li = queue_ref_top( &current->exit_waiters );
		     li != (list *)&current->exit_waiters;
		     li = next) {
		
			next = li->next;

			cthr = CONTAINER_OF(li, thread, parent_link);
			if ( wflags & THR_WAIT_ID ) {

				/*  指定されたTIDのスレッド以外は受け付けない */
				if ( cthr->tid != wait_tid ) 
					continue;

				if ( !( cthr->thr_flags & THR_FLAG_JOINABLE ) ) {

					spinlock_unlock_restore_intr( &current->lock, &flags );
					return -ENOENT;  /*  終了を待ち合わせないスレッド  */
				}
					
			} else if ( (wflags & THR_WAIT_PROC ) && ( cthr->p != current->p ) )
				continue;  /*  待ち合わせ対象のスレッドでない */

			goto found;  /*  処理対象が見つかった  */
		}
	}
found:
	spinlock_unlock_restore_intr( &current->lock, &flags );

	kassert( cthr != NULL );

	list_del( &cthr->parent_link );
	cthr->thr_flags &= ~THR_FLAG_JOINABLE;

	current->resource.children_sys_time += cthr->resource.sys_time;
	current->resource.children_user_time += cthr->resource.user_time;
	
	*rcp = cthr->exit_code;
	*exit_tidp = cthr->tid;

	sync_wake( &cthr->parent_wait, SYNC_WAI_RELEASED);

	return 0;
}
/** CPUを開放してリスケジュールする
 */
void
thr_yield(void) {
	intrflags flags;

	hal_cpu_disable_interrupt(&flags);
	current->status = THR_TSTATE_READY;    /*  スレッドの状態をTHR_TSTATE_READYに遷移  */
	if ( ti_dispatch_disabled(ti_get_current_tinfo()) ) {
		
		/* スケジュール不能区間だった場合は, 遅延ディスパッチフラグを
		 * 立てて抜ける
		 */
		ti_set_delay_dispatch(ti_get_current_tinfo());
		goto unmask_out; 
	}

	sched_schedule();

unmask_out:
	hal_cpu_restore_interrupt(&flags);
}
