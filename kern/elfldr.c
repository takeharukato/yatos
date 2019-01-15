/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  ELF loading routines                                              */
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
#include <kern/proc.h>
#include <kern/vm.h>
#include <kern/page.h>
#include <kern/elfldr.h>

//#define DEBUG_ELF_LOAD_FROM_MEMORY

static void
phflags2str(uint32_t p_flags, char *str) {

	str[3]='\0';
	if (p_flags & PF_X) 
		str[2] = 'X';	
	else 
		str[2] = ' ';

	if (p_flags & PF_W) 
		str[1] = 'W';	
	else 
		str[1] = ' ';

	if (p_flags & PF_R) 
		str[0] = 'R';	
	else 
		str[0] = ' ';

}
/** ELFのプログラムヘッダ中のフラグをVMAのプロテクト値に変換する
    @param[in] ph_prot プログラムヘッダ中のプロテクト情報
    @return VMAのプロテクト値
 */
static vma_prot
ph_prot2vma_prot(Elf64_Word p_flags) {
	vma_prot vprot;

	vprot = 0;
	if (p_flags & PF_R) 
		vprot |= VMA_PROT_R;
	if (p_flags & PF_W) 
		vprot |= VMA_PROT_W;
	if (p_flags & PF_X) 
		vprot |= VMA_PROT_X;

	return vprot;
}

/** メモリ上のELFファイルイメージをユーザ空間に配置する
    @param[in] pp      対象のプロセス情報の保存先
    @param[in] thrp    生成したスレッドへのポインタ
    @param[in] kvaddr  モジュールのメモリ領域(カーネル仮想アドレス)
    @retval  0     正常終了
 */
int
_proc_load_ELF_from_memory(proc* p, void *kvaddr){
	Elf_Ehdr          *hdr;
	Elf_Phdr         *phdr;
	int           i, c, rc;
	uintptr_t vaddr, paddr;
	void           *uvaddr;
	void         *new_page;
	vma              *vmap;
	char            flg[4];
	intrflags        flags;

	kassert( p != NULL );
	kassert( kvaddr != NULL );
	kassert( p->text == NULL );
	kassert( p->data == NULL );

	hdr = kvaddr;

	if (memcmp(hdr->e_ident, ELFMAG, SELFMAG)) 
		return -EINVAL;

#if defined(DEBUG_ELF_LOAD_FROM_MEMORY)
	kprintf(KERN_DBG, "entry: %p offset: 0x%lx, size:%lx num:%d\n", 
	    hdr->e_entry, hdr->e_phoff, hdr->e_phentsize, hdr->e_phnum);
#endif  /*  DEBUG_ELF_LOAD_FROM_MEMORY  */

	spinlock_lock_disable_intr( &p->lock , &flags );

	p->entry = (int (*)(void *))hdr->e_entry;

	phdr = kvaddr + hdr->e_phoff;
	for(i = 0, c = 0; i < hdr->e_phnum; ++i) {

		phflags2str(phdr->p_flags, flg);

#if defined(DEBUG_ELF_LOAD_FROM_MEMORY)
		kprintf(KERN_DBG, 
		    "PHdr(type, flags, file-offset, vaddr, fsize, msize)="
		    "(%x, %x(%s), %x, %p, %d, %u)\n",
		    phdr->p_type, phdr->p_flags, flg, phdr->p_offset, phdr->p_vaddr, 
		    phdr->p_filesz, phdr->p_memsz);
#endif  /*  DEBUG_ELF_LOAD_FROM_MEMORY  */

		if ( phdr->p_type == PT_LOAD ) {	
			
			if ( c == 0 ) {

				rc = vm_create_vma(&p->vm, 
				    &p->text, 
				    (void *)phdr->p_vaddr,
				    ( PAGE_ALIGNED(phdr->p_vaddr + phdr->p_memsz) ?
					( phdr->p_memsz ) :
					PAGE_NEXT(phdr->p_memsz) ),
				    ph_prot2vma_prot(phdr->p_flags),
				    VMA_FLAG_FIXED);
				if ( rc != 0 ) {
					goto no_page_out;
				}
				
				vmap = p->text;

			} else if (c == 1) {

				rc = vm_create_vma(&p->vm, 
				    &p->data, 
				    (void *)phdr->p_vaddr,
				    ( PAGE_ALIGNED(phdr->p_vaddr + phdr->p_memsz) ?
					( phdr->p_memsz ) :
					PAGE_NEXT(phdr->p_memsz) ),
				    ph_prot2vma_prot(phdr->p_flags),
				    VMA_FLAG_FIXED);
				if ( rc != 0 ) 
					goto unmap_pages_out;

				vmap = p->data;

			} else
				continue;

			/*
			 * map user pages
			 */
			for(vaddr = (uintptr_t)vmap->start, 
				    paddr =(uintptr_t)( kvaddr + phdr->p_offset );  
			    vaddr < PAGE_NEXT( phdr->p_vaddr + phdr->p_filesz ); 
			    vaddr += PAGE_SIZE, paddr += PAGE_SIZE) {
				
				rc = get_free_page(&new_page);
				if ( rc != 0 )
					goto unmap_pages_out;
				
				memcpy(new_page, (void *)paddr, PAGE_SIZE);
				kassert( *( (uintptr_t *)new_page ) == 
				    *( (uintptr_t *)paddr ) );
				kassert( 
					*( (uintptr_t *)(new_page + 
						PAGE_SIZE - sizeof(uintptr_t) ) ) == 
					*( (uintptr_t *)(paddr  + 
						PAGE_SIZE - sizeof(uintptr_t) ) ) );
				
				rc = hal_map_user_page(&p->vm, (uintptr_t)vaddr, 
				    (uintptr_t)new_page, vmap->prot );
				if ( rc != 0 )
					goto unmap_pages_out;
			}

			/*
			 * メモリ上にのみ有り, ファイルにないページ
			 * (BSS領域)
			 */
			for(vaddr = PAGE_NEXT( phdr->p_vaddr + phdr->p_filesz ),
				    paddr =(uintptr_t)( kvaddr + phdr->p_offset );  
			    vaddr < (uintptr_t)vmap->end; 
			    vaddr += PAGE_SIZE, paddr += PAGE_SIZE) {

				rc = get_free_page(&new_page);
				if ( rc != 0 )
					goto unmap_pages_out;

				memset(new_page, 0, PAGE_SIZE);
				
				rc = hal_map_user_page(&p->vm, (uintptr_t)vaddr, 
				    (uintptr_t)new_page, vmap->prot );
				if ( rc != 0 )
					goto unmap_pages_out;
			}

			++c;  /* Increment loaded segments  */
		} 
		++phdr;
	}

	spinlock_unlock_restore_intr( &p->lock , &flags );	
	return 0;

unmap_pages_out:
	/*
	 * データ領域の開放
	 */
	if ( p->data != NULL ) {

		for( uvaddr = p->data->start; uvaddr < p->data->end; uvaddr += PAGE_SIZE) 
			vm_unmap_addr(&p->vm, uvaddr);

		kfree(p->data);
		p->data = NULL;
	}

	/*
	 * テキスト領域の開放
	 */
	kassert( p->text != NULL );
	for( uvaddr = p->text->start; uvaddr < p->text->end; uvaddr += PAGE_SIZE) 
		vm_unmap_addr(&p->vm, uvaddr);

	kfree(p->text);
	p->text = NULL;

no_page_out:
	p->entry = NULL;
	spinlock_unlock_restore_intr( &p->lock , &flags );
	return rc;
}
