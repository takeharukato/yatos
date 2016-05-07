/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  process relevant definitions                                      */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_PROC_H)
#define  _KERN_PROC_H 

#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/rbtree.h>
#include <kern/queue.h>
#include <kern/spinlock.h>
#include <kern/thread.h>
#include <kern/async-event.h>
#include <kern/vm.h>

/** プロセスの状態
 */
typedef enum _proc_state{
	PROC_PSTATE_DORMANT=0,     /*< プロセス停止中              */
	PROC_PSTATE_ACTIVE=1,      /*< 実行中                      */
	PROC_PSTATE_EXIT=2,        /*< 終了                        */
}proc_state;

struct _thread;
struct _vma;
/** プロセス
 */
typedef struct _proc{
	spinlock            lock;  /*< プロセス全体のロック          */
	list                link;  /*< リンク情報                    */
	RB_ENTRY(_proc)    mnode;  /*< プロセス一覧の赤黒木のノード  */
	pid                  pid;  /*< プロセスID                    */
	proc_state        status;  /*< プロセスの状態                */
	void              *entry;  /*< 開始アドレス                  */
	vm                    vm;  /*< 仮想アドレス空間              */
	struct _thread   *master;  /*< マスタスレッド                */
	queue            threads;  /*< プロセス内のスレッド群        */
	event_queue        evque;  /*< イベントキュー                */
	void        *u_evhandler;  /*< 共通イベントハンドラアドレス  */
	struct _vma        *text;  /*< テキストの仮想メモリ領域      */
	struct _vma        *data;  /*< データの仮想メモリ領域        */
	struct _vma        *heap;  /*< ヒープの仮想メモリ領域        */
	struct _vma       *stack;  /*< スタックの仮想メモリ領域      */
}proc;

/** プロセス辞書
 */
typedef struct _proc_dic{
	spinlock                     lock;  /*< 辞書排他用のロック    */
	RB_HEAD(proc_dic, _proc) booking;  /*< プロセス一覧のヘッド  */
}proc_dic;

#define __PROC_DIC_INITIALIZER(root)				\
	{							\
	.lock=__SPINLOCK_INITIALIZER,			        \
	.booking = RB_INITIALIZER(root),                        \
	}

int proc_create(proc **_procp, thr_prio prio, char *cmdline, 
    const char *environ[], void *_exec_image);
int proc_start(proc *_proc);
int proc_destroy(proc *_proc);
int proc_expand_stack(proc *_p, void *_new_top);
int proc_expand_heap(proc *_p, void *_new_heap_end, void **_old_heap_endp);
bool active_proc_locked_by_self(void);
void acquire_active_proc_lock(intrflags *_flags);
void release_active_proc_lock(intrflags *_flags);

void hal_free_user_page_table(vm *_as);
int hal_map_user_page(vm *_as, uintptr_t _vaddr, uintptr_t _paddr, vma_prot _prot);
int hal_unmap_user_page(vm *_as, uintptr_t _vaddr);
int hal_translate_user_page(vm *_as, uintptr_t _vaddr, uintptr_t *_kvaddrp, vma_prot *_protp);
int hal_alloc_user_page_table(vm *_as);
void hal_load_system_procs(void);
#endif  /*  _KERN_PROC_H   */
