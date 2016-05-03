#if !defined(KERN_ASM_OFFSET_HELPER_H)
#define KERN_ASM_OFFSET_HELPER_H
#include <stdint.h>
#include <stddef.h>

#define __asm_offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define DEFINE_SIZE(sym, val) 						  \
        asm volatile("\n.ascii \"@ASM_OFFSET@" #sym " %0 " #val "\"" : : "i" (val)) \

#define OFFSET(sym, str, mem) \
	DEFINE_SIZE(sym, __asm_offsetof(struct str, mem))
#endif  /*  KERN_ASM_OFFSET_HELPER_H  */
