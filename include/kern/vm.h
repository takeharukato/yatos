/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual memory area relevant definitions                          */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_VM_H)
#define  _KERN_VM_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>
#include <kern/rbtree.h>
#include <kern/mutex.h>

#include <hal/kernlayout.h>
#include <hal/userlayout.h>
#include <hal/pgtbl.h>

#define VMA_PROT_NONE   (0)  /*<  保護属性無し/ページ不在 */
#define VMA_PROT_R      (1)  /*<  読み取り可能            */
#define VMA_PROT_W      (2)  /*<  書き込み可能            */
#define VMA_PROT_X      (4)  /*<  実行可能                */

#define VMA_FLAG_FIXED  (0)  /*<  領域長固定                              */
#define VMA_FLAG_HEAP   (1)  /*<  ヒープ領域(アドレスの大きい方に伸長)    */
#define VMA_FLAG_STACK  (2)  /*<  スタック領域(アドレスの小さい方に伸長)  */

struct _proc;
typedef struct _vm{
	mutex                      asmtx;  /*< 仮想アドレス空間のmutex  */
	struct _proc                  *p;  /*< プロセスへの参照         */
	void                      *pgtbl;  /*< ページテーブル           */
	RB_HEAD(vma_tree, _vma) vma_head;  /*< VMAのヘッド              */
}vm;

typedef struct _vma{
	spinlock       lock;  /*< 仮想アドレス領域のロック                */
	struct _vm      *as;  /*< 仮想空間への参照                        */
	RB_ENTRY(_vma) node;  /*< 赤黒木のノード                          */
	void         *start;  /*< 仮想アドレス領域の開始アドレス          */
	void           *end;  /*< 仮想アドレス領域の終了アドレス          */
	vma_prot       prot;  /*< 仮想アドレス領域の保護属性              */
	vma_flags     flags;  /*< 仮想アドレス領域の種別(ヒープ/スタック) */
}vma;

void vm_init(vm *_as, struct _proc *_p);
void vm_destroy(vm *_as);
int vm_find_vma(vm *_as, void *_vaddr, vma **_res);
int vm_map_addr(struct _vm *_as, void *_vaddr, void *_kpaddr);
int vm_map_newpage(struct _vm *_as, void *_vaddr, vma_prot _prot);
int vm_unmap_addr(struct _vm *_as, void *_vaddr);
int vm_create_vma(vm *_as, struct _vma **_vmapp, void *_start, 
    size_t _size, vma_prot _prot, vma_flags _vflags);
int vm_destroy_vma(struct _vma *_vmap);
int vm_resize_area(vm *_as, void *_fault_addr, void *_new_addr, void **_old_addrp);
int vm_copy_in(vm *_as, void *_dest, const void *_src, size_t _count);
int vm_copy_out(vm *_as, void *_dest, const void *_src, size_t _count);
bool vm_user_area_can_access(vm *as, void *start, size_t count, vma_prot prot);
#endif  /*  _KERN_VM_H   */
