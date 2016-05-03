/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  architecture dependent page table definitions                     */
/*                                                                    */
/**********************************************************************/
#if !defined(__HAL_PGTBL_H)
#define __HAL_PGTBL_H

#include <hal/kernlayout.h>
#include <hal/arch-page.h>

#define PT_INDEX_SHIFT          (12)
#define PD_INDEX_SHIFT          (21)
#define PDPT_INDEX_SHIFT        (30)
#define PML4_INDEX_SHIFT        (39)
#define PT_INDEX_MASK           (0x1ff)
#define PD_INDEX_MASK           (0x1ff)
#define PDPT_INDEX_MASK         (0x1ff)
#define PML4_INDEX_MASK         (0x1ff)
#define PT_INDEX(vaddr)         ( ( (vaddr) >> PT_INDEX_SHIFT   ) & PT_INDEX_MASK)
#define PD_INDEX(vaddr)         ( ( (vaddr) >> PD_INDEX_SHIFT   ) & PD_INDEX_MASK)
#define PDPT_INDEX(vaddr)       ( ( (vaddr) >> PDPT_INDEX_SHIFT ) & PDPT_INDEX_MASK)
#define PML4_INDEX(vaddr)       ( ( (vaddr) >> PML4_INDEX_SHIFT ) & PML4_INDEX_MASK)
#define PGTBL_ENTRY_SIZE        (0x8)
#define PGTBL_ENTRY_MAX         ( PAGE_SIZE / (0x8) )

#define PAGE_PRESENT            (1 << 0)
#define PAGE_WRITABLE           (1 << 1)
#define PAGE_USER               (1 << 2)
#define PAGE_WRITETHROUGH       (1 << 3)
#define PAGE_NONCACHABLE        (1 << 4)
#define PAGE_ACCESSED           (1 << 5)
#define PAGE_DIRTY              (1 << 6)
#define PAGE_2MB                (1 << 7)
#define PAGE_GLOBAL             (1 << 8)

#define PAGE_PGTBLBITS          ( PAGE_PRESENT | PAGE_WRITABLE )

#if !defined(ASM_FILE)

#include <stdint.h>

#define PAGE_EXECUTE_DISABLE    (1ULL << 63)  /* int型の幅を超えるため !ASM_FILEが必要  */

typedef uintptr_t pml4e;
typedef uintptr_t pdpe;
typedef uintptr_t pdire;
typedef uintptr_t pte;

typedef struct _pml4_tbl {
	pml4e entries[PGTBL_ENTRY_MAX];
}pml4_tbl;

typedef struct _pdp_tbl {
	pdpe entries[PGTBL_ENTRY_MAX];
}pdp_tbl;

typedef struct _pdir_tbl {
	pdire entries[PGTBL_ENTRY_MAX];
}pdir_tbl;

typedef struct _pte_tbl {
	pte entries[PGTBL_ENTRY_MAX];
}pte_tbl;

typedef pml4_tbl pgtbl_t;

#define pdp_present(pml4_ent) ( (pml4_ent) & PAGE_PRESENT )
#define pdir_present(pdp_ent) ( (pdp_ent) & PAGE_PRESENT )
#define pte_present(pdir_ent) ( (pdir_ent) & PAGE_PRESENT )
#define page_present(pte_ent) ( (pte_ent) & PAGE_PRESENT )

static inline pml4e
get_pml4_ent(pgtbl_t *kpgtbl, uintptr_t vaddr) {
	pml4_tbl *pml4;

	pml4 = (pml4_tbl *)kpgtbl;
	return pml4->entries[PML4_INDEX(vaddr)];
}

static inline pdpe
get_pdp_ent(pdp_tbl *pdptbl, uintptr_t vaddr) {

	return pdptbl->entries[PDPT_INDEX(vaddr)];
}

static inline pdire
get_pdir_ent(pdir_tbl *pdirtbl, uintptr_t vaddr) {

	return pdirtbl->entries[PD_INDEX(vaddr)];
}

static inline pte
get_pte_ent(pte_tbl *ptetbl, uintptr_t vaddr) {

	return ptetbl->entries[PT_INDEX(vaddr)];
}

static inline uintptr_t
get_ent_addr(uintptr_t ent) {

	return PAGE_START(ent);
}

static inline uintptr_t
get_ent_attr(uintptr_t ent) {

	return ( (ent) & ( (uintptr_t)(PAGE_SIZE) - (uintptr_t)1 ) );
}

static inline void
set_pml4_ent(pgtbl_t *kpgtbl, uintptr_t vaddr, uintptr_t val) {
	pml4_tbl *pml4;

	pml4 = (pml4_tbl *)kpgtbl;
	pml4->entries[PML4_INDEX(vaddr)] = val;
}

static inline void
set_pdp_ent(pdp_tbl *pdptbl, uintptr_t vaddr, uintptr_t val) {

	pdptbl->entries[PDPT_INDEX(vaddr)] = val;
}

static inline void
set_pdir_ent(pdir_tbl *pdirtbl, uintptr_t vaddr, uintptr_t val) {

	pdirtbl->entries[PD_INDEX(vaddr)] = val;
}

static inline void
set_pte_ent(pte_tbl *ptetbl, uintptr_t vaddr, uintptr_t val) {

	ptetbl->entries[PT_INDEX(vaddr)] = val;
}

static inline uintptr_t
read_cr2(void){
	uintptr_t v;

	__asm__ __volatile__("mov %%cr2, %0\n\t" : "=r" (v));
	    return v;
}

static inline uintptr_t
read_cr3(void){
	uintptr_t v;

	__asm__ __volatile__("mov %%cr3, %0\n\t" : "=r" (v));
	    return v;
}

static inline void
write_cr3(uintptr_t v){

	__asm__ __volatile__("mov %0, %%cr3" : : "r" (v));
}

static inline void
invalidate_tlb(void){
	
	write_cr3(read_cr3());
}

static inline void 
load_pgtbl(const uintptr_t pgtbl_addr) {

	write_cr3(pgtbl_addr);

	return;
}
#endif  /*  !ASM_FILE  */

#endif  /*  __HAL_PGTBL_H  */
