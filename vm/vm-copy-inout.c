/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  User Copy in/out relevant routines                                */
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
#include <kern/proc.h>
#include <kern/thread.h>
#include <kern/vm.h>
#include <kern/page.h>
#include <kern/mutex.h>

#include <vm/vm-internal.h>

/**  ユーザ空間へのアクセス許可があることを確認する(実装部)
     @param[in] as    検査対象の仮想アドレス空間
     @param[in] start 検査開始アドレス
     @param[in] count 検査範囲のアドレス長(単位:バイト)
     @param[in] prot  検査するアクセス権(仮想メモリ領域の保護属性)
     @retval    true  対象範囲に物理ページが存在し, 必要なアクセス権がある
     @retval    false 対象範囲に物理ページが存在しないか, 必要なアクセス権がない
 */
static bool
user_area_can_access_nolock(vm *as, void *start, size_t count, vma_prot prot) {
	int             rc;
	void     *pg_start;
	void       *pg_end;
	vma          *vmap;

	kassert( as != NULL );

	pg_start = (void *)PAGE_START((uintptr_t)start);
	pg_end = ( PAGE_ALIGNED( (uintptr_t)(start + count) ) ? 
	    ( start + count ) : ( (void *)PAGE_NEXT( (uintptr_t)(start + count) ) ) );

	rc = _vm_find_vma_nolock(as, pg_start, &vmap);
	if ( rc != 0 )
		goto can_not_access;

	if ( vmap->end < pg_end )
		goto can_not_access;

	if ( !( vmap->prot & prot ) )
		goto can_not_access;

	return true;

can_not_access:
	return false;
}

/** プロセス間でデータをコピーする
    @param[in] dest_as  コピー先の仮想アドレス空間
    @param[in] src_as   コピー元の仮想アドレス空間
    @param[in] dest     コピー先のアドレス
    @param[in] src      コピー元のアドレス
    @param[in] count    コピーするバイト数
    @return    コピーしたバイト数
    @return    -ENOMEM バッファページが獲得できない
    @return    -EFAULT ページが存在しない
    @note      カーネルストレートマップ領域間でメモリコピーを行うことで
               アドレス空間の切り替えを行わないようにする
	       また、自プロセスの空間のmutexを取ると互いの空間のロック待ちで
	       デッドロックするため, プリエンプションを禁止にして自プロセス内の
	       他のスレッドが割り込まないようにし, 自プロセスのアドレス空間の
	       mutexを取らないようにする。
 */
static int
inter_user_copy(vm *dest_as, vm *src_as, void *dest, const void *src, size_t count) {
	int                 rc;
	size_t             len;
	uintptr_t   src_kvaddr;
	vma_prot      src_prot;
	uintptr_t  dest_kvaddr;
	vma_prot     dest_prot;
	void            *saddr;
	void            *daddr;
	size_t         src_len;
	size_t        dest_len;
	size_t         cpy_len;
	void         *new_page;
	vma              *vmap;

	kassert( src_as != NULL );
	kassert( src_as->p != NULL );
	kassert( dest_as != NULL );
	kassert( dest_as->p != NULL );

	/*  アドレス空間とアドレスの範囲がユーザ空間に収まることを確認  */
	if ( ( (uintptr_t)src >= KERN_VMA_BASE ) || ( (uintptr_t)dest >= KERN_VMA_BASE ) )
		return -EFAULT;

	if ( ( dest_as->p == hal_refer_kernel_proc() ) ||
	    ( src_as->p == hal_refer_kernel_proc() ) )
		return -EFAULT;

	/*  自CPUの同一プロセス内のスレッドがアドレス空間を操作
	 *   しないことを保証するためにプリエンプションを抑止
	 */
	ti_disable_dispatch();  

	/*  相手の空間のロック(mutex)を取る  */
	if ( &current->p->vm == dest_as ) 
		mutex_lock( &src_as->asmtx );
	else 
		mutex_lock( &dest_as->asmtx );
	
	saddr = (void *)src;
	daddr = dest;

	for( len = count; len > 0; ) {
		
		/*
		 * コピー元/コピー先ページを取得する
		 */
		if ( src_as->p == hal_refer_kernel_proc() )
			src_kvaddr = PAGE_START(saddr);
		else {

			rc = hal_translate_user_page(src_as, (uintptr_t)saddr,
			    &src_kvaddr, &src_prot);
			if ( rc != 0 ) {
				
				/*
				 * ページ未割り当て時はページを割当てる
				 */
				rc = _vm_find_vma_nolock(src_as, saddr, &vmap);
				if ( rc != 0 )
					goto unlock_out;

				if ( vmap->prot == VMA_PROT_NONE )
					goto unlock_out;

				rc = get_free_page(&new_page);
				if ( rc != 0 )
					goto unlock_out;

				memset(new_page, 0, PAGE_SIZE);
				rc = hal_map_user_page(src_as, (uintptr_t)saddr, 
				    (uintptr_t)new_page, vmap->prot );
				if ( rc != 0 ) {
					
					free_page(new_page);
					goto unlock_out;
				}
			}
		}

		if ( dest_as->p == hal_refer_kernel_proc() )
			dest_kvaddr = PAGE_START(daddr);
		else {

			rc = hal_translate_user_page(dest_as, (uintptr_t)daddr,
			    &dest_kvaddr, &dest_prot);
			if ( rc != 0 ) {
				
				/*
				 * ページ未割り当て時はページを割当てる
				 */
				rc = _vm_find_vma_nolock(dest_as, daddr, &vmap);
				if ( rc != 0 )
					goto unlock_out;

				if ( vmap->prot == VMA_PROT_NONE )
					goto unlock_out;

				rc = get_free_page(&new_page);
				if ( rc != 0 )
					goto unlock_out;

				memset(new_page, 0, PAGE_SIZE);
				rc = hal_map_user_page(dest_as, (uintptr_t)daddr, 
				    (uintptr_t)new_page, vmap->prot );
				if ( rc != 0 ) {
					
					free_page(new_page);
					goto unlock_out;
				}
			}
		}

		/*
		 * src/destともページ内に収まる分だけコピーする
		 */
		src_len = PAGE_NEXT(saddr) - (uintptr_t)saddr;
		dest_len = PAGE_NEXT(daddr) - (uintptr_t)daddr;
		cpy_len = ( src_len <= dest_len ) ? ( src_len ) : ( dest_len );
		if ( cpy_len > len )
			cpy_len = len;

		len -= cpy_len;
		while( cpy_len > 0 ) {

			*(char *)(dest_kvaddr + ( (uintptr_t)daddr - PAGE_START(daddr) ) ) 
				= *(char *)(src_kvaddr +
				    ( (uintptr_t)saddr - PAGE_START(saddr) ) );
			--cpy_len;
			++saddr;
			++daddr;
		}
	}

	rc = count - len;

unlock_out:
	if ( &current->p->vm == dest_as ) 
		mutex_unlock( &src_as->asmtx );
	else 
		mutex_unlock( &dest_as->asmtx );

	ti_enable_dispatch();

	return rc;
}

/**  ユーザ空間へのアクセス許可があることを確認する
     @param[in] as    検査対象の仮想アドレス空間
     @param[in] start 検査開始アドレス
     @param[in] count 検査範囲のアドレス長(単位:バイト)
     @param[in] prot  検査するアクセス権(仮想メモリ領域の保護属性)
     @retval    true  対象範囲に物理ページが存在し, 必要なアクセス権がある
     @retval    false 対象範囲に物理ページが存在しないか, 必要なアクセス権がない
 */
bool
vm_user_area_can_access(vm *as, void *start, size_t count, vma_prot prot) {
	bool         rc;

	if ( as->p == hal_refer_kernel_proc() )
		return true;

	mutex_lock( &as->asmtx );
	rc = user_area_can_access_nolock(as, start, count, prot);
	mutex_unlock( &as->asmtx );

	return rc;
}

/** 仮想アドレス空間上のアドレスからカーネル空間にコピーする
    @param[in] as  コピー元の仮想アドレス空間
    @param[in] dest  コピー先のアドレス
    @param[in] src   コピー元のアドレス
    @param[in] count コピーするバイト数
    @return    コピーしたバイト数
    @return    -EFAULT ページが存在しない
 */
int
vm_copy_in(vm *as, void *dest, const void *src, size_t count) {
	int          rc;

	kassert( as != NULL );
	kassert( as->p != NULL );

	if ( current->p != hal_refer_kernel_proc() ) {  /* ユーザへのコピー  */
		
		if ( (uintptr_t)dest < KERN_VMA_BASE ) {

			/*
			 * ユーザプロセス間コピー
			 */
			rc = inter_user_copy(&current->p->vm, as, dest, src, count);
			if ( rc < 0 )
				return rc;
			count = rc;
			goto copy_end;
		}
	}

	mutex_lock( &as->asmtx );

	/*
	 * カーネルへのコピー
	 */
	if ( as->p == hal_refer_kernel_proc() )
		goto copy_ok;  /*  カーネルからのコピーなのでアクセス可能  */

	/*
	 * コピー元ユーザアドレスの妥当性確認
	 */
	if ( (uintptr_t)src  >= KERN_VMA_BASE ) {
		
		rc = -EFAULT;
		goto error_out;
	}
	if ( !user_area_can_access_nolock(as, (void *)src, count, VMA_PROT_R ) ) {

		rc = -EFAULT;
		goto error_out;
	}
copy_ok:
	memcpy(dest, src, count);
	mutex_unlock( &as->asmtx );
copy_end:
	return count;

error_out:
	mutex_unlock( &as->asmtx );
	return rc;
}

/** 仮想アドレス空間上のアドレスへカーネル空間の内容をコピーする
    @param[in] as    コピー先の仮想アドレス空間
    @param[in] dest  コピー先のアドレス
    @param[in] src   コピー元のアドレス
    @param[in] count コピーするバイト数
    @return    コピーしたバイト数
    @return    -EFAULT ページが存在しない
 */
int
vm_copy_out(vm *as, void *dest, const void *src, size_t count) {
	int          rc;

	kassert( as != NULL );
	kassert( as->p != NULL );

	if ( current->p != hal_refer_kernel_proc() ) {  /* ユーザからのコピー  */

		if ( (uintptr_t)src < KERN_VMA_BASE )  {

			/*
			 * ユーザプロセス間コピー
			 */
			rc = inter_user_copy(as, &current->p->vm, dest, src, count);
			if ( rc < 0 )
				return rc;
			count = rc;
			goto copy_end;
		}
	}

	mutex_lock( &as->asmtx );

	/*
	 * カーネルからのコピー
	 */
	if ( as->p == hal_refer_kernel_proc() ) 
		goto copy_ok;  /* カーネルへのコピーなのでアクセス可能  */

	/*
	 * コピー先ユーザアドレスの妥当性確認
	 */
	if ( ( (uintptr_t)dest ) >= KERN_VMA_BASE ) {

		rc = -EFAULT;
		goto error_out;
	}
	if ( !user_area_can_access_nolock(as, (void *)dest, count, VMA_PROT_W ) ) {

		rc = -EFAULT;
		goto error_out;
	}

copy_ok:
	memcpy(dest, src, count);
	mutex_unlock( &as->asmtx );

copy_end:
	return count;

error_out:
	mutex_unlock( &as->asmtx );
	return rc;
}
