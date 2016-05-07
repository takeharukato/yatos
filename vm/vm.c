/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual Memory Area relevant routines                             */
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

static int vma_cmp(struct _vma *_a, struct _vma *_b);

RB_GENERATE_STATIC(vma_tree, _vma, node, vma_cmp);

/** 仮想アドレス領域の比較
    @param[in] a 比較対象の仮想アドレス領域1
    @param[in] b 比較対象の仮想アドレス領域2
    @retval 0  比較対象の仮想アドレス領域1と比較対象の仮想アドレス領域2の開始アドレスが等しい
    @retval 負 仮想アドレス領域1の方がアドレス空間中の前にある
    @retval 正 仮想アドレス領域1の方がアドレス空間中の後ろにある
 */
static int 
vma_cmp(struct _vma *a, struct _vma *b) {

	kassert( (a != NULL) && (b != NULL) );

	if ( a->start < b->start )
		return -1;

	if ( a->start > b->start )
		return 1;

	return 0;
}

/** 仮想アドレス領域データを獲得する
    @param[in] as      生成先のアドレス空間
    @param[in] vmapp   生成した仮想アドレス空間の返却先
    @param[in] start   仮想アドレス空間の先頭アドレス
    @param[in] end     仮想アドレス空間の末尾アドレス
    @param[in] prot    仮想アドレス空間の保護属性
    @param[in] vflags  仮想アドレス空間の種別
    @retval  vma構造体へのポインタ  正常に生成
    @retval  NULL                   メモリが獲得できなかった
 */
static vma *
alloc_new_vma(vm *as, void *start, void *end, vma_prot prot, vma_flags vflags){
	vma *vmap;

	vmap = kmalloc( sizeof(vma), KMALLOC_NORMAL );
	if ( vmap == NULL )
		return NULL;

	spinlock_init( &vmap->lock );
	vmap->as = as;
	vmap->start = start;
	vmap->end = end;
	vmap->prot = prot;
	vmap->flags = vflags;

	return vmap;
}

/**  指定された仮想アドレス領域と重なるアドレス空間が登録されていないことを確認する
     @param[in] as    探査対象のアドレス空間
     @param[in] vmap  比較対象の仮想アドレス領域
     @retval    0     指定された仮想アドレス領域と重なるアドレス空間が登録されていない
     @retval   -EBUSY 指定された仮想アドレス領域と重なるアドレス空間が登録されている
 */
static int
check_vma_overlap_nolock(vm *as, vma *vmap) {
	int         rc;
	void *pg_start;
	void   *pg_end;
	vma   *vma_ref;

	kassert( as != NULL );
	kassert( mutex_locked_by_self(&as->asmtx) );
	kassert( vmap != NULL );

	pg_start = (void *)PAGE_START((uintptr_t)vmap->start);
	pg_end = ( PAGE_ALIGNED((uintptr_t)vmap->end) ? 
	    ( vmap->end ) : ( (void *)PAGE_NEXT((uintptr_t)vmap->end) ) );

#if defined(DEBUG_SHOW_VMA)
	kprintf(KERN_DBG, "check_vma (start, end)=(%p,%p)\n", pg_start, pg_end);
#endif  /*  DEBUG_SHOW_VMA  */

	rc = -EBUSY;

	if ( (uintptr_t)pg_end >= KERN_VMA_BASE ) 
		goto error_out;

	RB_FOREACH(vma_ref, vma_tree, &as->vma_head) {
		
		if ( ( vma_ref->start <= pg_start ) && ( pg_start < vma_ref->end ) ) 
			goto error_out;
		
		if ( ( vma_ref->start <= pg_end ) && ( pg_end <= vma_ref->end ) ) 
			goto error_out;

		if ( ( pg_start <= vma_ref->start ) && ( vma_ref->start < pg_end ) )
			goto error_out;

		if ( ( pg_start < vma_ref->end ) && ( vma_ref->end <= pg_end ) ) 
			goto error_out;
	}

	rc = 0;

error_out:
	return rc;
}

/**  指定されたアドレスを含むVMAを検索する(実装部)
     @param[in] as    検索対象のアドレス空間
     @param[in] vaddr 検索キーとなるアドレス
     @param[out] res  見つかったvmaのポインタを格納する先(NULLの場合はVMAを返却せず復帰)
     @retval     0       正常に見つかった
     @retval    -ENOENT  指定したアドレスを含む領域を見つけられなかった
 */
int
_vm_find_vma_nolock(vm *as, void *vaddr, vma **res) {
	int          rc;
	vma    *vma_ref;

	kassert( as != NULL );
	kassert( mutex_locked_by_self(&as->asmtx) );
	kassert( res != NULL );

	RB_FOREACH(vma_ref, vma_tree, &as->vma_head) {
		
		spinlock_lock( &vma_ref->lock );
		if ( ( ( vma_ref->start <= vaddr ) && ( vaddr < vma_ref->end ) ) || 
		    ( ( vma_ref->start == vma_ref->end ) && ( vaddr == vma_ref->start ) ) ) {

			if ( res != NULL )
				*res = vma_ref;

			rc = 0;
			spinlock_unlock( &vma_ref->lock );
			goto found;
		}
		spinlock_unlock( &vma_ref->lock );
	}
	
	rc = -ENOENT;
	
found:
	return rc;
}

/** 指定された範囲のページ割当てを解除する
    @param[in] as    操作対象のアドレス空間
    @param[in] start 開始ページ
    @param[in] end   終了ページ
    @retval    0     正常終了
    @note endを「含まない」ページまでをアンマップすることに注意
    @note vm_map_newpage_rangeと同様に呼べるように0を返す仕様とした.
 */
static int 
unmap_range(vm *as, void *start, void *end) {
	void      *vaddr;

	kassert( as != NULL );
	kassert( mutex_locked_by_self(&as->asmtx) );

	for( vaddr = start; vaddr < end; vaddr += PAGE_SIZE) {
		
		hal_unmap_user_page(as, (uintptr_t)vaddr); 
	}
	
	return 0;
}

/** 指定された範囲に新規ページを割り当てる
    @param[in] as     操作対象のアドレス空間
    @param[in] start  開始ページ
    @param[in] end    終了ページ
    @param[in] prot   保護属性
    @retval    0      正常に割当て済み
    @retval   -ENOMEM ページテーブル/新規ページ割当て失敗
    @note endを「含まない」ページまでにマップすることに注意

 */
static int
map_newpage_range(vm *as, void *start, void *end, vma_prot prot) {
	int           rc;
	void      *vaddr;
	void   *new_page;

	kassert( as != NULL );
	kassert( mutex_locked_by_self(&as->asmtx) );

	for( vaddr = start; vaddr < end; vaddr += PAGE_SIZE) {

		rc = get_free_page(&new_page);
		if ( rc != 0 ) {
			
			unmap_range(as, start, vaddr);  /*  マップ済みページを解放する  */
			goto error_out;
		}

		memset(new_page, 0, PAGE_SIZE);
				
		rc = hal_map_user_page(as, (uintptr_t)vaddr, 
		    (uintptr_t)new_page, prot );

		if ( rc != 0 ) {

			unmap_range(as, start, vaddr); /*  マップ済みページを解放する  */
			free_page(new_page);  /*  未マップページを解放する */
			goto error_out;
		}
	}

	return 0;

error_out:
	return rc;
}

/** 仮想アドレス領域を伸縮する
    @param[in] vmap   操作対象の仮想アドレス領域
    @param[in] start  変更後の開始アドレス
    @param[in] end    変更後の終了アドレス
    @retval    0      正常に変更した
    @retval   -ENOMEM メモリ不足により変更に失敗した
    @note     他の領域との衝突がないか確認するのは呼出元責任。
              heapの縮小など呼出元でないと判断できないため。
 */
static int
resize_vma(vma *vmap, void *start, void *end) {
	int           rc;
	void  *new_start;
	void    *new_end;
	intrflags  flags;

	kassert( vmap != NULL);
	kassert( vmap->as != NULL);
	kassert( start != NULL );
	kassert( mutex_locked_by_self(&vmap->as->asmtx) );

	/*
	 * 引数で与えられたアドレスをページ境界に合わせる
	 */
	new_start = (void *)( PAGE_ALIGNED( (uintptr_t)start ) ? 
	    ( (uintptr_t)start ) : ( PAGE_START( (uintptr_t)start ) ) );
	new_end =  (void *)( PAGE_ALIGNED( (uintptr_t)end ) ? 
	    ( (uintptr_t)end ) : ( PAGE_NEXT( (uintptr_t)end ) ) );

	if ( new_end < new_start )
		return -EINVAL;

	spinlock_lock_disable_intr( &vmap->lock , &flags );

	rc = 0;

	if ( vmap->start != new_start ) {  /*  スタック操作  */

		if ( vmap->start > new_start ) 
			rc = map_newpage_range(vmap->as, new_start, 
			    vmap->start, vmap->prot);
		else 
			rc = unmap_range(vmap->as, vmap->start, new_start);
	} 

	if ( vmap->end != new_end ) { /*  ヒープ操作  */
		
		if ( vmap->end < new_end )
			rc = map_newpage_range(vmap->as, vmap->end, 
			    new_end, vmap->prot);
		else 
			rc = unmap_range(vmap->as, new_end, vmap->end);
	}

	if ( rc != 0 )
		goto unlock_out;

	vmap->start = new_start;
	vmap->end = new_end;

	rc = 0;

unlock_out:
	spinlock_unlock_restore_intr( &vmap->lock , &flags );

	return rc;
}

/** 仮想アドレス領域を破棄する(実処理部)
    @param[in] vmap 捜査対象の仮想アドレス領域
    @retval 0 正常に破棄した
 */
static int
vm_destroy_vma_nolock(vma *vmap){
	int           rc;
	void      *vaddr;
	
	kassert( vmap != NULL );
	kassert( vmap->as != NULL );
	kassert( vmap != NULL );

	for( vaddr = vmap->start; vaddr < vmap->end; vaddr += PAGE_SIZE){

		rc = hal_unmap_user_page(vmap->as, (uintptr_t)vaddr);
		kassert( rc == 0 );
	}

	kfree( vmap );  /*  vma領域のページを解放する  */

	return 0;
}

/**  指定されたアドレスを含むVMAを検索する
     @param[in] as    検索対象のアドレス空間
     @param[in] vaddr 検索キーとなるアドレス
     @param[out] res  見つかったvmaのポインタを格納する先(NULLの場合はVMAを返却せず復帰)
     @retval     0       正常に見つかった
     @retval    -ENOENT  指定したアドレスを含む領域を見つけられなかった
 */
int
vm_find_vma(vm *as, void *vaddr, vma **res) {
	int          rc;

	kassert( as != NULL );
	kassert( res != NULL );

	mutex_lock( &as->asmtx );
	rc = _vm_find_vma_nolock(as, vaddr, res);
	mutex_unlock( &as->asmtx );

	return rc;
}

/**  srcで示される仮想アドレス領域の内容をdestで示される仮想アドレス領域のオフセット位置に
     格納する
     @param[in] dest    コピー先の仮想アドレス領域
     @param[in] src     コピー元の仮想アドレス領域
     @retval     0      正常にコピーできた
     @retval    -ENOMEM メモリの割当てに失敗した
 */
int
vm_copy_vma(vma *dest, vma *src) {
	int             rc;
	intrflags    flags;
	void        *saddr;
	void        *daddr;
	void     *new_page;
	uintptr_t old_page;
	vma_prot  old_prot;

	kassert( src != NULL );
	kassert( dest != NULL );
	kassert( src->as != NULL );
	kassert( dest->as != NULL );
	kassert( PAGE_ALIGNED( (uintptr_t)src->start ) );
	kassert( PAGE_ALIGNED( (uintptr_t)src->end ) );
	kassert( PAGE_ALIGNED( (uintptr_t)dest->start ) );
	kassert( PAGE_ALIGNED( (uintptr_t)dest->end ) );

	dest->prot = src->prot;
	dest->flags = src->flags;

	spinlock_lock_disable_intr( &src->lock , &flags );
	spinlock_lock( &dest->lock );
	
	/*
	 * ページが割り当てられている領域をコピーする
	 */
	for( saddr = src->start, daddr = dest->start;
	     ( ( saddr < src->end ) && ( daddr < dest->end ) ); 
	     saddr += PAGE_SIZE, daddr += PAGE_SIZE ) {

		rc = hal_translate_user_page(src->as, (uintptr_t)saddr, &old_page, &old_prot);
		if ( rc == 0 ) {

			rc = get_free_page( &new_page );
			if ( rc != 0 )
				goto error_out;
			
			memcpy(new_page, (void *)old_page, PAGE_SIZE); 
			rc = hal_map_user_page(dest->as, (uintptr_t)daddr, 
			    (uintptr_t)new_page, dest->prot );
		}
	}

error_out:
	spinlock_unlock( &dest->lock );
	spinlock_unlock_restore_intr( &src->lock , &flags );

	return rc;
}


/** 仮想アドレス領域を生成する
    @param[in] as      生成先のアドレス空間
    @param[in] vmapp   生成した仮想アドレス空間の返却先
    @param[in] start   仮想アドレス空間の先頭アドレス
    @param[in] size    仮想アドレス空間のサイズ
    @param[in] prot    仮想アドレス空間の保護属性
    @param[in] vflags  仮想アドレス空間の種別
    @retval  0         正常に生成
    @retval -EBUSY     既に領域が使用されている
 */
int
vm_create_vma(vm *as, vma **vmapp, void *start, size_t size, vma_prot prot, vma_flags vflags){
	int          rc;
	vma *vmap, *res;

	kassert( as != NULL );
      	kassert( mutex_locked_by_self(&as->asmtx) );
	kassert( vmapp != NULL );

	vmap = alloc_new_vma(as, start, start + size, prot, vflags);
	if ( vmap == NULL )
		return -ENOMEM;

	rc = check_vma_overlap_nolock(as, vmap);
	if ( rc != 0 ) 
		goto free_vma_out;

	res = RB_INSERT(vma_tree, &as->vma_head, vmap);
	kassert( res == NULL );

	*vmapp =vmap;

	return 0;

free_vma_out:
	kfree( vmap );
	return rc;
}

/** 仮想アドレス領域を破棄する
    @param[in] vmap 捜査対象の仮想アドレス領域
    @retval 0 正常に破棄した
 */
int
vm_destroy_vma(vma *vmap){
	int           rc;
	
	kassert( vmap != NULL );
	kassert( vmap->as != NULL );
	kassert( vmap != NULL );

	mutex_lock( &vmap->as->asmtx );
	rc = vm_destroy_vma_nolock(vmap);
	mutex_unlock( &vmap->as->asmtx );
	return rc;
}

/** 仮想空間を複製する
    @param[in] dest 複製先の仮想空間
    @param[in] src  複製元の仮想空間
 */
void
vm_duplicate(vm *dest, vm *src) {
	int           rc;
	vma        *vmap;
	vma     *new_vma;

	kassert( dest != NULL );
	kassert( src != NULL );
	kassert( dest->pgtbl != NULL );
	kassert( src->pgtbl != NULL );
	kassert( dest->p != NULL );
	kassert( src->p != NULL );
	kassert( dest->p != hal_refer_kernel_proc() );
	kassert( src->p != hal_refer_kernel_proc() );

	mutex_lock( &src->asmtx );
	RB_FOREACH(vmap, vma_tree, &src->vma_head) {

		rc = vm_create_vma(dest, &new_vma, vmap->start, 
		    vmap->end - vmap->start, vmap->prot, vmap->flags);
		if ( rc != 0 )
			goto error_out;

		rc = vm_copy_vma(new_vma, vmap);
		if ( rc != 0 )
			goto error_out;
	}
error_out:
	mutex_unlock( &src->asmtx );

}

/** 仮想アドレス空間の所定の仮想アドレスに指定した物理メモリをマップする
    @param[in] as    操作対象の仮想アドレス空間
    @param[in] vaddr マップする仮想アドレス
    @param[in] kpaddr マップする物理メモリのカーネル仮想アドレス
    @retval       0  正常にマップした
    @retval -EFAULT  不正な仮想アドレスにマップしようとした
    @retval -ENOMEM  ページテーブルのメモリ獲得に失敗した
 */
int
vm_map_addr(vm *as, void *vaddr, void *kpaddr){
	int           rc;
	vma         *res;

	kassert( as != NULL );
	kassert( mutex_locked_by_self(&as->asmtx) );

	if ( ( (uintptr_t)vaddr ) >= USER_VADDR_LIMIT )
		return -EFAULT;

	rc = _vm_find_vma_nolock(as, vaddr, &res);
	if ( rc != 0 ) 
		goto error_out;

	rc = hal_map_user_page(as, (uintptr_t)vaddr, 
	    (uintptr_t)kpaddr, res->prot ); 

error_out:
	return rc;
}

/** 仮想アドレス空間の所定の仮想アドレスに新規ページを割り当てる
    @param[in] as    操作対象の仮想アドレス空間
    @param[in] vaddr マップする仮想アドレス
    @param[in] prot  マップ時のVMA保護属性
    @retval       0  正常にマップした
    @retval -ENOMEM  ページテーブルのメモリ獲得に失敗した
                     新規ページの獲得に失敗した
 */
int
vm_map_newpage(vm *as, void *vaddr, vma_prot prot){
	int           rc;
	void   *new_page;
	vma         *res;

	kassert( as != NULL );
	kassert( mutex_locked_by_self(&as->asmtx) );

	if ( prot == VMA_PROT_NONE )
		return 0;

	if ( ( (uintptr_t)vaddr ) >= USER_VADDR_LIMIT )
		return -EFAULT;

	rc = _vm_find_vma_nolock(as, vaddr, &res);
	if ( rc != 0 )
		goto error_out;

	rc = get_free_page(&new_page);
	if ( rc != 0 ) 
		goto error_out;

	memset(new_page, 0, PAGE_SIZE);

	rc = hal_map_user_page(as, (uintptr_t)vaddr, 
	    (uintptr_t)new_page, res->prot );

error_out:

	return rc;
}

/** 仮想アドレス空間の所定の仮想アドレスの仮想<->物理変換を解除する
    @param[in] as    操作対象の仮想アドレス空間
    @param[in] vaddr マップする仮想アドレス
    @retval       0  正常にマップした
    @retval -EFAULT  不正な仮想アドレスをアンマップしようとして
 */
int
vm_unmap_addr(vm *as, void *vaddr){
	int           rc;
	vma         *res;

	kassert( as != NULL );
	kassert( mutex_locked_by_self(&as->asmtx) );

	if ( ( (uintptr_t)vaddr ) >= USER_VADDR_LIMIT )
		return -EFAULT;

	rc = _vm_find_vma_nolock(as, vaddr, &res);
	if ( rc != 0 ) 
		goto error_out;

	hal_unmap_user_page(as, (uintptr_t)vaddr);

error_out:
	return rc;
}

/** ヒープ/スタック領域を伸長/縮小する
    @param[in] as         操作対象の仮想アドレス空間
    @param[in] fault_addr ページフォルトアドレス/ヒープ開始アドレスを指定
    @param[in] new_addr   更新後のアドレス
    @param[in] old_addrp   更新前のアドレス
 */
int
vm_resize_area(vm *as, void *fault_addr, void *new_addr, void **old_addrp){
	int               rc;
	void       *old_addr;
	vma             *res;
	void      *new_start;
	void        *new_end;

	kassert( as != NULL );
	kassert( mutex_locked_by_self(&as->asmtx) );
	kassert( fault_addr != NULL );
	kassert( new_addr != NULL );
	kassert( old_addrp != NULL );

	if ( ( (uintptr_t)fault_addr ) >= USER_VADDR_LIMIT )
		return -EFAULT;

	if ( ( (uintptr_t)new_addr ) >= USER_VADDR_LIMIT )
		return -EFAULT;

	rc = _vm_find_vma_nolock(as, fault_addr, &res);
	if ( rc != 0 ) 
		goto error_out;

	if ( res->flags & VMA_FLAG_HEAP ) {
		
		old_addr = res->end;

		new_start = res->start;
		new_end =  (void *)( PAGE_ALIGNED( (uintptr_t)new_addr ) ? 
		    ( (uintptr_t)new_addr ) : ( PAGE_NEXT( (uintptr_t)new_addr ) ) );

		rc = _vm_find_vma_nolock(as, new_end, &res);
		if ( ( old_addr < new_end ) && ( rc == 0 ) )  { /*  他の領域の衝突する  */

			rc = -EBUSY;
			goto error_out;
		}
	} else if ( res->flags & VMA_FLAG_STACK ) {
		
		old_addr = res->start;
		new_start = (void *)( PAGE_ALIGNED( (uintptr_t)new_addr ) ? 
		    ( (uintptr_t)new_addr ) : ( PAGE_START( (uintptr_t)new_addr ) ) );
		new_end = res->end;

		rc = _vm_find_vma_nolock(as, new_start, &res);
		if ( ( old_addr > new_start ) && ( rc == 0 ) ) { /*  他の領域の衝突する  */

			rc = -EBUSY;
			goto error_out;
		}
	} else {

		/*  伸縮不能領域  */
		rc = -EPERM;
		goto error_out;
	}

	rc = resize_vma(res, new_start, new_end);	
	if ( rc == 0 ) {

		*old_addrp = old_addr;
	}
error_out:
	return rc;
}

/** 仮想アドレス空間を初期化し, ユーザページテーブルディレクトリを割り当てる 
    @param[in] as  操作対象の仮想空間
    @param[in] p   仮想空間が所属するプロセス
 */
void
vm_init(vm *as, proc *p){

	kassert( as != NULL );
	kassert( p != NULL );

        /*
	 *  プロセス空間にユーザページテーブルディレクトリを割り当てる 
	 */
	hal_alloc_user_page_table( as );
}

/** 仮想空間を破棄し, ページテーブルを解放する
    @param[in] as  操作対象の仮想空間
 */
void
vm_destroy(vm *as) {
	vma *cur, *next;

	kassert( as != NULL );
	kassert( spinlock_locked_by_self(&as->p->lock) );

	mutex_lock( &as->asmtx );
	/*
	 * VMAを全て破棄する
	 */
	for (cur = RB_MIN(vma_tree, &as->vma_head); cur != NULL; cur = next) {

		next = RB_NEXT(vma_tree, &as->vma_head, cur);
		RB_REMOVE(vma_tree, &as->vma_head, cur);
		vm_destroy_vma_nolock(cur);
	}

	hal_free_user_page_table( as );  /*  ページテーブルを解放   */

	mutex_unlock( &as->asmtx );

	as->pgtbl = NULL;
}
