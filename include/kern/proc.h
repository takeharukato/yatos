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
#include <kern/queue.h>
#include <kern/spinlock.h>
#include <kern/thread.h>
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
typedef struct _proc{
	spinlock          lock;
	list              link;
	pid                pid;
	proc_state      status;
	void            *entry;
	vm                  vm;
	struct _thread *master;
	queue          threads;
	void      *u_evhandler;
	struct _vma      *text;
	struct _vma      *data;
	struct _vma      *heap;
	struct _vma     *stack;
}proc;

typedef struct _proc_queue{
	spinlock lock;
	queue     que;
}proc_queue;

#define __PROC_QUEUE_INITIALIZER(_que)				\
	{							\
	.lock=__SPINLOCK_INITIALIZER,			        \
	.que = __QUEUE_INITIALIZER(_que),	                \
	}

int proc_create(proc **_procp, thr_prio prio, char *cmdline, 
    const char *environ[], void *_exec_image);
int proc_start(proc *_proc);
int proc_destroy(proc *_proc);
int proc_expand_stack(proc *_p, void *_new_top);
int proc_expand_heap(proc *_p, void *_new_heap_end, void **_old_heap_endp);

void hal_free_user_page_table(vm *_as);
int hal_map_user_page(vm *_as, uintptr_t _vaddr, uintptr_t _paddr, vma_prot _prot);
int hal_unmap_user_page(vm *_as, uintptr_t _vaddr);
int hal_translate_user_page(vm *_as, uintptr_t _vaddr, uintptr_t *_kvaddrp, vma_prot *_protp);
int hal_alloc_user_page_table(vm *_as);
void hal_load_system_procs(void);
#endif  /*  _KERN_PROC_H   */
