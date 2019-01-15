/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  ELF format relevant definitions                                   */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_ELF_GENERIC_H)
#define _KERN_ELF_GENERIC_H

#include <stdint.h>
#include <stddef.h>

/*
 * Definitions of generic ELF names which relieve applications from
 * needing to know the word size.
 */

#if __ELF_WORD_SIZE != 32 && __ELF_WORD_SIZE != 64
#error "__ELF_WORD_SIZE must be defined as 32 or 64"
#endif

#define	ELF_CLASS	__CONCAT(ELFCLASS,__ELF_WORD_SIZE)

#if !defined(__BYTE_ORDER__)
#error "We assume that __BYTE_ORDER__ is defined by compilers."
#endif  /*  __BYTE_ORDER__  */

#if !defined(__ORDER_LITTLE_ENDIAN__) && !defined(__ORDER_BIG_ENDIAN__) 
#error "We assume that either __ORDER_LITTLE_ENDIAN__ or __ORDER_BIG_ENDIAN__ is defined by compilers."
#endif  /*  __BYTE_ORDER__  */

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define	ELF_DATA	ELFDATA2LSB
#elif  __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define	ELF_DATA	ELFDATA2MSB
#else
#error "Unknown byte order"
#endif

#define	__ELF_GENERIC_CONCAT1(x,y)	x ## y
#define	__ELF_GENERIC_CONCAT(x,y)	__ELF_GENERIC_CONCAT1(x,y)

#define	__elfN(x)	\
	__ELF_GENERIC_CONCAT(__ELF_GENERIC_CONCAT(__ELF_GENERIC_CONCAT(elf,__ELF_WORD_SIZE),_),x)
#define	__ElfN(x)	\
	__ELF_GENERIC_CONCAT(__ELF_GENERIC_CONCAT(__ELF_GENERIC_CONCAT(Elf,__ELF_WORD_SIZE),_),x)
#define	__ELFN(x)	\
	__ELF_GENERIC_CONCAT(__ELF_GENERIC_CONCAT(__ELF_GENERIC_CONCAT(ELF,__ELF_WORD_SIZE),_),x)
#define	__ElfType(x)	\
	typedef __ElfN(x) __ELF_GENERIC_CONCAT(Elf_,x)

__ElfType(Addr);
__ElfType(Half);
__ElfType(Off);
__ElfType(Sword);
__ElfType(Word);
__ElfType(Ehdr);
__ElfType(Shdr);
__ElfType(Phdr);
__ElfType(Dyn);
__ElfType(Rel);
__ElfType(Rela);
__ElfType(Sym);
__ElfType(Verdef);
__ElfType(Verdaux);
__ElfType(Verneed);
__ElfType(Vernaux);
__ElfType(Versym);

/* Non-standard ELF types. */
__ElfType(Hashelt);
__ElfType(Size);
__ElfType(Ssize);

#define	ELF_R_SYM	__ELFN(R_SYM)
#define	ELF_R_TYPE	__ELFN(R_TYPE)
#define	ELF_R_INFO	__ELFN(R_INFO)
#define	ELF_ST_BIND	__ELFN(ST_BIND)
#define	ELF_ST_TYPE	__ELFN(ST_TYPE)
#define	ELF_ST_INFO	__ELFN(ST_INFO)
#define	ELF_ST_VISIBILITY	__ELFN(ST_VISIBILITY)

#endif  /*  _KERN_ELF_GENERIC_H  */
