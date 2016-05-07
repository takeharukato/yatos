/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Process relevant routines                                         */
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
#include <kern/rbtree.h>
#include <kern/id-bitmap.h>
#include <kern/proc.h>
#include <kern/vm.h>
#include <kern/thread.h>
#include <kern/async-event.h>
#include <kern/page.h>
#include <kern/ctype.h>
#include <kern/mutex.h>

#include <proc/proc-internal.h>

/* 停止プロセス  */
static proc_queue proc_dormant_queue = __PROC_QUEUE_INITIALIZER( &proc_dormant_queue.que );
/* 動作中プロセス */
static proc_queue proc_active_queue = __PROC_QUEUE_INITIALIZER( &proc_active_queue.que );

/** コマンドライン解析器の状態
 */
typedef enum { 
	START=1,
	PRINTABLE=2,
	SPACE=3,
	TERM=4,
}cmdline_parser_state;

/** コマンドライン解析情報
 */
typedef struct _cmdline_parser_arg{
	int     cnt;  /*<  コマンドライン引数の数                      */
	char *start;  /*<  コマンドライン開始位置                      */
	char    *cp;  /*<  コマンドライン解析位置                      */
	void   *arg;  /*<  コマンドライン解析コールバック関数への引数  */
}cmdline_parser_arg;

/**  スタック領域を割り当てる
     @param[in] p       操作対象のプロセス
     @retval    0       正常に割り当てた
     @retval   -ENOMEM  メモリ不足により割り当てに失敗した
 */
static int
alloc_process_stack(proc *p, size_t size){
	int           rc;
	void   *new_page;
	void     *uvaddr;

	/*
	 * stack
	 */
	mutex_lock( &p->vm.asmtx );
	rc = vm_create_vma(&p->vm, 
	    &p->stack, 
	    (void *)(USER_STACK_BOTTOM - PAGE_SIZE),
	    size,
	    VMA_PROT_R | VMA_PROT_W | VMA_PROT_X,
	    VMA_FLAG_STACK);
	if ( rc != 0 )
		goto unlock_out;

	rc = get_free_page(&new_page);
	if ( rc != 0 )
		goto  unmap_stack_out;

	memset(new_page, 0, size);
				
	rc = hal_map_user_page(&p->vm, (uintptr_t)p->stack->start, (uintptr_t)new_page, 
	    p->stack->prot );
	if ( rc != 0 )
		goto unmap_stack_out;

	rc = 0;

	mutex_unlock( &p->vm.asmtx );
	return rc;

unmap_stack_out:
	for( uvaddr = p->stack->start; uvaddr < p->stack->end; uvaddr += PAGE_SIZE) 
		vm_unmap_addr(&p->vm, uvaddr);
	kfree(p->stack);
	p->stack = NULL;

unlock_out:
	mutex_unlock( &p->vm.asmtx );
	return rc;
}

/**  コマンドライン文字列の引数の数を数える
     @param[in] old コマンドライン解析の字句解析情報(状態機械の前の状態)
     @param[in] new コマンドライン解析の字句解析情報(状態機械の現在の状態)
     @param[in] arg コマンドライン解析情報
 */
void
count_args_cb(cmdline_parser_state old , cmdline_parser_state new, cmdline_parser_arg *arg) {

	kassert( arg != NULL );

	if (old == START)
		arg->cnt = 1;
	else if ( ( old == SPACE ) && ( new == PRINTABLE ) )
		++arg->cnt;
}

/**  コマンドライン文字列を空白文字の位置で分離する
     @param[in] old コマンドライン解析の字句解析情報(状態機械の前の状態)
     @param[in] new コマンドライン解析の字句解析情報(状態機械の現在の状態)
     @param[in] arg コマンドライン解析情報
 */
void
split_args_cb(cmdline_parser_state old , cmdline_parser_state new, cmdline_parser_arg *arg) {
	void **argv;

	kassert( arg != NULL );
	argv = (void **)arg->arg;

	if (old == START)
		arg->cnt = 1;
	else if ( old == SPACE ) {
		if  ( new == PRINTABLE ) {

			argv[arg->cnt] = (void *)(arg->cp);
			++arg->cnt;
		}
	}
	if (new == SPACE)
		*(arg->cp) = '\0';
}

/** コマンドライン文字列を解析する
    @param[in,out] str コマンドライン文字列
    @param[in] cb      コールバック関数
    @param[in] parg    引数解析情報
    @retval  0         正常に解析完了
    @retval -ENOENT    引数がない
 */
static int
parse_arg_string(char *str, void (*cb)(cmdline_parser_state , cmdline_parser_state , 
	cmdline_parser_arg *), cmdline_parser_arg *parg){
	cmdline_parser_state state;

	parg->cnt = 0;
	parg->cp = NULL;

	/*  最初の非空白文字を探査  */
	for(parg->cp = str; isspace(*parg->cp) ; ++parg->cp);
	if ( *parg->cp == '\0' )
		return -ENOENT;
	
	/*
	 * 文字列を調査する
	 */
	parg->start = parg->cp;
	state=PRINTABLE;
	if ( cb != NULL )
		cb(START, state, parg);

	for(parg->cp = parg->start; ( *parg->cp != '\0' ); ++parg->cp ){

		switch( state ) {
		case START:
			break;
		case TERM:
			break;
		case PRINTABLE:
			if ( isspace(*parg->cp) ) {

				state = SPACE;
				if ( cb != NULL )
					cb(PRINTABLE, SPACE, parg);
			} else if ( cb != NULL )
				cb(PRINTABLE, PRINTABLE, parg);
			continue;
		case SPACE:
			if ( ( !isspace(*parg->cp) ) && ( *parg->cp != '\0' )) {

				state = PRINTABLE;
				if ( cb != NULL )
					cb(SPACE, PRINTABLE, parg);
			} else if ( cb != NULL )
				  cb(SPACE, SPACE, parg);
			continue;
		}
	}

	if ( cb != NULL )
		cb(state, TERM, parg);

	return 0;
}

/** プロセスの初期スタックを構築する
    @param[in] p           プロセス情報
    @param[in] paramp      引数文字列へのポインタ
    @param[in] env         引き渡す環境変数
    @param[in] argcp       引数の数を返却する領域
    @param[in] argvp       引数配列を返却する領域
    @param[in] env_arrayp  環境変数配列を返却する領域
    @param[in] mp          プロセススタックの初期値を返却する領域
    @retval    0           正常に設定完了
    @retval   -ENOENT      パラメタがない(少なくともコマンド名はなければならない)
 */
static int
setup_process_stack(proc *p, const char *paramp, const char *env[], int *argcp, 
    void **argvp, void **env_arrayp,
    void **mp) {
	int                  rc;
	void            *ustack;
	void             **argv;
	void        **env_array;
	void            *uvaddr;
	char              *bufp;
	char              *envp;
	int                   i;
	size_t           envsiz;
	size_t           envlen;
	size_t             size;
	size_t              len;
	size_t          nr_args;
	size_t          nr_envs;
	cmdline_parser_arg parg;

	/*
	 * アーギュメントの数を調べる
	 */
	parg.arg = NULL;
	rc = parse_arg_string((char *)paramp, count_args_cb, &parg);
	if ( rc != 0 )
		return rc;

	nr_args = parg.cnt + 1; /* 最後のNULL格納領域のため+1する  */

	/*
	 * 環境変数格納領域長と数を調べる
	 */
	nr_envs = 0;  /*  環境変数の数  */
	for(envsiz = 0, i = 0; ( env != NULL ) && ( env[i] != NULL ); ++i) {
		
		envsiz += strlen( env[i] ) + 1;  /* NULLターミネート含む  */
		++nr_envs;
	}
	++nr_envs;  /* 最後のNULL格納領域  */

	/*
	 * 引数文字列をコピーする
	 */
	len = strlen(parg.start);

        /*  スタックサイズを計算する  */
	size = PAGE_NEXT( len + 1 + sizeof(char *) * (nr_args) +
	    envsiz + sizeof(char *) * nr_envs );  

	rc = alloc_process_stack(p, size);
	kassert(rc == 0);

	ustack = p->stack->start;

	memset(ustack, 0, size);

#if defined(DEBUG_CMDLINE_PARSER)
	kprintf(KERN_DBG, "Argc=%d size=%u envsiz=%u\n", parg.cnt, 
	    (unsigned int)size, (unsigned int)envsiz);
#endif  /*  DEBUG_CMDLINE_PARSER  */

	/*  環境変数配列を設定 */
	env_array = ustack + size - 
		envsiz -  /*  環境変数文字列分  */
		( len + 1 ) -  /*  引数文字列分  */
		sizeof(char *) * nr_envs;
	/*  環境変数をコピー  */
	for(envp = ustack + size -  envsiz, i = 0;
	    ( env != NULL ) && ( env[i] != NULL ); 
	    ++i) {

		envlen = strlen( env[i] ) + 1;
		memcpy(envp, env[i], envlen);
		envp[envlen - 1]='\0';
		env_array[i] = envp;
		envp += envlen;  /* NULLターミネート含む  */
	}
	env_array[nr_envs - 1] = NULL;

	bufp = ustack + size -  ( envsiz + len + 1 ); /*  引数文字列をコピー  */
	memcpy(bufp, parg.start, len + 1);  

        /* argvを設定  */
	argv = ustack + size - 
		envsiz -  /*  環境変数文字列分  */
		( len + 1 ) -  /*  引数文字列分  */
		sizeof(char *) * nr_envs -  /* 環境変数配列分  */
		( sizeof(void *) * nr_args ); /*  引数配列分  */
	argv[0] = bufp;

	parg.arg = (void *)&argv[0];
	rc = parse_arg_string((char *)bufp, split_args_cb, &parg);
	if ( rc != 0 )
		goto free_mem_out;
	argv[nr_args -1] = NULL;

	rc = 0;

	*mp = argv;
	*argvp = argv;
	*env_arrayp = env_array;
	*argcp = parg.cnt;

	return rc;

free_mem_out:
	for( uvaddr = p->stack->start; uvaddr < p->stack->end; uvaddr += PAGE_SIZE) 
		vm_unmap_addr(&p->vm, uvaddr);
	kfree(p->stack);
	p->stack = NULL;

	return rc;
}

/**  ヒープ領域を割り当てる
     @param[in] p       操作対象のプロセス
     @retval    0       正常に割り当てた
     @retval   -ENOMEM  メモリ不足により割り当てに失敗した
 */
static int
setup_heap_area(proc *p){
	int           rc;

	/*
	 * heap
	 */
	mutex_lock( &p->vm.asmtx );

	rc = vm_create_vma(&p->vm, 
	    &p->heap, 
	    (void *)PAGE_NEXT( (uintptr_t)(p->data->end) ),
	    0,
	    VMA_PROT_R | VMA_PROT_W,
	    VMA_FLAG_HEAP);

	mutex_unlock( &p->vm.asmtx );

	return rc;
}

/** スタック範囲を拡大する
        @param[in] p          プロセス構造体
        @param[in] new_top    拡大後のスタックのトップ
	@retval    0          スタック拡大に成功した
	@retval   -EBUSY      他の領域と衝突する
	@retval   -ENOMEM     メモリ不足
 */
int
proc_expand_stack(proc *p, void *new_top){
	int  rc;
	void *old_addr;

	kassert( p != NULL );
	kassert( p->stack != NULL );

	mutex_lock( &p->vm.asmtx );
	rc = vm_resize_area(&p->vm, p->stack->start, new_top, &old_addr);
	mutex_unlock( &p->vm.asmtx );

	return rc;
}

/** ヒープ範囲を伸縮する
        @param[in]  p              プロセス構造体
        @param[in]  new_heap_end   変更後のheapの最終位置
	                           NULLの場合は伸縮せず現在の値を返す
        @param[out] old_heap_endp  変更前のheapの最終位置返却アドレス
	@retval    0               ヒープの更新に成功した
	@retval   -EBUSY           他の領域と衝突する
	@retval   -ENOMEM          メモリ不足
 */
int
proc_expand_heap(proc *p, void *new_heap_end, void **old_heap_endp) {
	int  rc;
	void *old_addr;

	kassert( p != NULL );
	kassert( p->heap != NULL );
	kassert( old_heap_endp != NULL );

	mutex_lock( &p->vm.asmtx );
	if ( new_heap_end != NULL )
		rc = vm_resize_area(&p->vm, p->heap->start, new_heap_end, &old_addr);
	else {
		
		old_addr = p->heap->end;
		rc = 0;
	}
	mutex_unlock( &p->vm.asmtx );
	if ( rc == 0 )
		*old_heap_endp = old_addr;

	return rc;
}
/** プロセスの基本情報を初期化
    @param[in] p  操作対象のプロセス
 */
void
_proc_common_init(proc *p) {

	kassert( p != NULL );

	spinlock_init( &p->lock );        /*  プロセスロックを初期化                 */
	list_init( &p->link );            /*  プロセスリンクを初期化                 */
	queue_init( &p->threads );        /*  プロセスロックとスレッドキューを初期化 */
	p->pid = THR_INVALID_TID;         /*  PIDを一時的に無効なIDに設定            */
	p->status = PROC_PSTATE_DORMANT;  /*  プロセスの状態を停止中に設定           */
	ev_queue_init( &p->evque );       /*  イベントキューを初期化                 */
	
	mutex_init( &p->vm.asmtx, MTX_FLAG_EXCLUSIVE);      /* 仮想空間mutexの初期化 */
	p->vm.p = p;                 /*  仮想空間の所属先プロセスを設定              */
	RB_INIT(&p->vm.vma_head);    /*  仮想空間の仮想メモリ領域ツリーを初期化する  */
}

/** プロセスを生成する
    @param[in] procp   プロセス構造体の返却先ポインタのアドレス
    @param[in] prio    スレッド優先度
    @param[in] cmdline コマンドライン文字列
    @param[in] environ 環境変数配列
    @param[in] image   実行形式ファイルロード先の先頭アドレス
                       (カーネルストレートマップアドレス)
    @retval 0 正常生成完了
 */
int
proc_create(proc **procp, thr_prio prio, char *cmdline, const char *environ[], void *image) {
	int          rc;
	intrflags flags;
	proc         *p;
	thread     *thr;
	int   proc_argc;
	void *proc_argv;
	void  *proc_env;
	void       *usp;
	void    *uvaddr;
	
        /*
	 * プロセス情報用のメモリを獲得する 
	 */
	p = kmalloc(sizeof(proc), KMALLOC_NORMAL);
	if ( p == NULL )
		return -ENOMEM;

	memset( p, 0, sizeof(proc) );

	_proc_common_init(p);             /*  プロセスの基本情報の初期化             */
	vm_init(&p->vm, p);               /*  仮想空間情報の初期化                   */

        /*
	 * ELFファイルを配置した領域からアドレス空間情報(VMA)を読込む 
	 */
	mutex_lock( &p->vm.asmtx );
	rc = _proc_load_ELF_from_memory(p, image);
	mutex_unlock( &p->vm.asmtx );
	if ( rc != 0 )
		goto free_proc_out;

	rc = setup_heap_area(p);      /*  ヒープ領域を割当てる    */
	if ( rc != 0 )
		goto unmap_text_and_data_out;

	/*
	 * プロセスのスタック領域に書き込むために一時的にアドレス空間を切り替える
	 * @note  vm_copy_in/vm_copy_outを使用すると文字列解析のため一時文字ずつコピー
	 * するかカーネル内のバッファにコピーした結果を転送することになるため,
	 * 他プロセスのTLBがフラッシュされる欠点があるが, アドレス空間を切り替えて, 
	 * ポインタ操作を行うことで, 処理ロジックを単純化する設計方針を採用。
	 */
	hal_switch_address_space( current->p,  p);
	/*   ユーザプロセス引数の設定  */
	rc = setup_process_stack(p, cmdline,environ , &proc_argc, &proc_argv, 
	    &proc_env, &usp);
	hal_switch_address_space( p, current->p);

	if ( rc != 0 )
		goto unmap_heap_out;

	rc = thr_new_thread(&thr);  /* プロセスのメインスレッドを作成 */
	if ( rc != 0 )
		goto unmap_stack_out;

        /* スレッドをユーザスレッドとして設定 */
	rc = thr_create_uthread(thr, prio, THR_FLAG_JOINABLE, p, p->entry, 
	    (uintptr_t)proc_argc, (uintptr_t)proc_argv, 
	    (uintptr_t)proc_env, (void *)usp);
	if ( rc != 0 )
		goto free_thread_out;

        /* 自スレッドをプロセスのマスタースレッドに設定 */
	p->master = thr;

	p->pid = thr->tid;  /*  最初スレッドのIDをプロセスIDに設定  */

        /* 停止プロセスキューに追加 */
	spinlock_lock_disable_intr( &proc_dormant_queue.lock, &flags );
	queue_add( &proc_dormant_queue.que, &p->link );
	spinlock_unlock_restore_intr( &proc_dormant_queue.lock, &flags );

	*procp = p;
	
	return 0;

free_thread_out:
	thr_destroy(thr);

unmap_stack_out:
	kassert( p->stack != NULL );

	/*
	 * スタック領域の開放
	 */
	for( uvaddr = p->stack->start; uvaddr < p->stack->end; uvaddr += PAGE_SIZE) 
		vm_unmap_addr(&p->vm, uvaddr);
	kfree(p->stack);
	p->stack = NULL;

unmap_heap_out:
	kassert( p->heap != NULL );

	/*
	 * ヒープ領域の開放
	 */
	for( uvaddr = p->heap->start; uvaddr < p->heap->end; uvaddr += PAGE_SIZE) 
		vm_unmap_addr(&p->vm, uvaddr);
	kfree(p->heap);
	p->heap = NULL;

unmap_text_and_data_out:
	kassert( p->text != NULL );
	kassert( p->data != NULL );

	/*
	 * データ領域の開放
	 */
	for( uvaddr = p->data->start; uvaddr < p->data->end; uvaddr += PAGE_SIZE) 
		vm_unmap_addr(&p->vm, uvaddr);
	kfree(p->data);
	p->data = NULL;

	/*
	 * テキスト領域の開放
	 */
	for( uvaddr = p->text->start; uvaddr < p->text->end; uvaddr += PAGE_SIZE) 
		vm_unmap_addr(&p->vm, uvaddr);
	kfree(p->text);
	p->text = NULL;

free_proc_out:
	kfree(p);
	return rc;
}

/** プロセスを開始する
    @param[in] proc 開始するプロセス構造体
    @retval  0      正常開始
    @retval -EBUSY  停止状態でないプロセスを開始しようとした
 */
int
proc_start(proc *p) {
	int          rc;
	intrflags flags;
	
	if ( p->status != PROC_PSTATE_DORMANT )
		return -EBUSY;

        /* 停止プロセスキューから削除 */
	spinlock_lock_disable_intr( &proc_dormant_queue.lock, &flags );
	list_del( &p->link );
	spinlock_unlock_restore_intr( &proc_dormant_queue.lock, &flags );
	
	/*  実行中キューに追加  */
	spinlock_lock_disable_intr( &proc_active_queue.lock, &flags );
	queue_add( &proc_active_queue.que, &p->link );
	spinlock_unlock_restore_intr( &proc_active_queue.lock, &flags );

	p->status = PROC_PSTATE_ACTIVE;  /* プロセスを動作中に遷移する  */
	
	rc = thr_start( p->master, current->tid );  /*  プロセスを開始する  */
	
	return rc;
}

/** プロセスを終了する
    @param[in] p プロセス情報
    @retval    0      正常終了
    @retval   -EBUSY  プロセス内にスレッドが残存する
 */
int 
proc_destroy(proc *p){
	int          rc;
	intrflags flags;

	kassert( p != NULL );

	/*  プロセス内にスレッドが居ればエラーで復帰  */
	spinlock_lock_disable_intr( &p->lock, &flags );
	if ( !queue_is_empty( &p->threads ) ) {

		rc = -EBUSY;
		spinlock_unlock_restore_intr( &p->lock, &flags );
		goto error_out;
	}

	if ( p->status == PROC_PSTATE_ACTIVE ) {

		/*  実行中キューから除去  */
		spinlock_lock_disable_intr( &proc_active_queue.lock, &flags );
		list_del( &p->link );
		spinlock_unlock_restore_intr( &proc_active_queue.lock, &flags );
	} else if ( p->status == PROC_PSTATE_DORMANT ) {
		
		/*  休止キューから除去  */
		spinlock_lock_disable_intr( &proc_dormant_queue.lock, &flags );
		list_del( &p->link );
		spinlock_unlock_restore_intr( &proc_dormant_queue.lock, &flags );
	}

	p->status = PROC_PSTATE_EXIT; /* これ以降スレッドを追加できないようにする  */

        /*
	 *  アドレス空間の解放
	 */
	vm_destroy( &p->vm );

	spinlock_unlock_restore_intr( &p->lock, &flags );

	rc = 0;

error_out:

	return rc;
}

/** 動作中プロセスのロックを獲得
    @param[in] flags 割込禁止状態保存領域
 */
void 
acquire_active_proc_lock(intrflags *flags) {

	spinlock_lock_disable_intr( &proc_active_queue.lock, flags );
}

/** 動作プロセスのロックを解放
    @param[in] flags 割込禁止状態保存領域
 */
void 
release_active_proc_lock(intrflags *flags) {

	spinlock_unlock_restore_intr( &proc_active_queue.lock, flags );
}

