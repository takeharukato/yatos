/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  preparation routines                                              */
/*                                                                    */
/**********************************************************************/
#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/thread.h>
#include <kern/sched.h>
#include <kern/proc.h>
#include <kern/vm.h>

#include <hal/prepare.h>
#include <hal/multiboot2.h>

//#define SHOW_MULTIBOOT_PARSE

/** デフォルトの環境変数情報
 */
static const char *x86_64_default_environ[] = {
	"TERM=yatos",
	NULL
};

/** GRUBのモジュールとして指定されたELFバイナリをロードする
 */
void
hal_load_system_procs(void) {
	int            i;
	int           rc;
	proc          *p;
	grub_mod    *mod;
	karch_info  *info = _refer_boot_info();
	
	for( i = 0; info->nr_mod > i; ++i) {
		
		mod = &info->modules[i];
		rc = proc_create(&p, 0, mod->param, x86_64_default_environ, 
		    (void *)PHY_TO_KERN_STRAIGHT(mod->start));
		kassert(rc == 0);

		rc = proc_start(p);
		kassert(rc == 0);
	}
}

/** GRUBのマルチブート情報を解析する
    @param[in] magic  マルチブートのマジック番号
    @param[in] mbaddr マルチブートのアドレス
    @param[in] info   HALのブート情報格納先アドレス
 */
void
parse_multiboot2_info(uint64_t magic, uint64_t mbaddr, karch_info *info){
	struct multiboot_tag     *tag;
	multiboot_memory_map_t  *mmap;
	grub_mod                 *mod;
	memory_area             *resv;
	memory_area              *ram;

	info->nr_resv = 0;
	info->nr_ram  = 0;
	info->nr_mod  = 0;

	if ( magic != MULTIBOOT2_BOOTLOADER_MAGIC ) {

		kprintf(KERN_CRI, "multiboot2 multiboot loader magic 0x%lx invalid.\n", 
		    magic);
		goto error_out;
	}		

#if defined(SHOW_MULTIBOOT_PARSE)
	kprintf(KERN_DBG, "multiboot2 loader magic=0x%lx ... valid.\n", magic);
#endif  /*  SHOW_MULTIBOOT_PARSE  */

	if (mbaddr & 7) {

		kprintf(KERN_CRI, "Unaligned mbi: 0x%x\n", mbaddr);
		goto error_out;
	}

#if defined(SHOW_MULTIBOOT_PARSE)
	kprintf(KERN_DBG, "Announced mbi size 0x%x at %p\n", 
	    *(unsigned *) mbaddr, (void *)(mbaddr + 8));
#endif  /*  SHOW_MULTIBOOT_PARSE  */
	
	for (tag = (struct multiboot_tag *) (mbaddr + 8);
	     tag->type != MULTIBOOT_TAG_TYPE_END;
	     tag = (struct multiboot_tag *) ((multiboot_uint8_t *)tag + 
		 ( (tag->size + 7) & ~7) )){

		switch (tag->type) {
		case MULTIBOOT_TAG_TYPE_CMDLINE:

#if defined(SHOW_MULTIBOOT_PARSE)
			kprintf(KERN_DBG, "Command line = %s\n",
				 ((struct multiboot_tag_string *) tag)->string);
#endif  /*  SHOW_MULTIBOOT_PARSE  */

			strncpy(&info->kparam[0], 
			    ((struct multiboot_tag_string *) tag)->string,
			    HAL_MB_PARAM_LEN - 1);
			info->kparam[HAL_MB_PARAM_LEN - 1] = '\0';
			break;
		case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:

#if defined(SHOW_MULTIBOOT_PARSE)
			kprintf(KERN_DBG, "Boot loader name = %s\n",
				((struct multiboot_tag_string *) tag)->string);
#endif  /*  SHOW_MULTIBOOT_PARSE  */
			break;
		case MULTIBOOT_TAG_TYPE_MODULE:

#if defined(SHOW_MULTIBOOT_PARSE)
			kprintf(KERN_DBG, "Module at 0x%x-0x%x. Command line %s\n",
				((struct multiboot_tag_module *) tag)->mod_start,
				((struct multiboot_tag_module *) tag)->mod_end,
			        ((struct multiboot_tag_module *) tag)->cmdline);
#endif  /*  SHOW_MULTIBOOT_PARSE  */

			if ( info->nr_mod == HAL_MAX_MB_MOD )
				continue;

			mod = &info->modules[info->nr_mod];
			strncpy(&mod->param[0], 
			    ((struct multiboot_tag_module *) tag)->cmdline,
			    HAL_MB_PARAM_LEN - 1);
			mod->param[HAL_MB_PARAM_LEN - 1] = '\0';
			mod->start = ((struct multiboot_tag_module *) tag)->mod_start;
			mod->end = ((struct multiboot_tag_module *) tag)->mod_end;
			++info->nr_mod;
			break;
		case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:

#if defined(SHOW_MULTIBOOT_PARSE)
			kprintf(KERN_DBG, "mem_lower = %u KB, mem_upper = %u KB\n",
			    ((struct multiboot_tag_basic_meminfo *) tag)->mem_lower,
			    ((struct multiboot_tag_basic_meminfo *) tag)->mem_upper);
#endif  /*  SHOW_MULTIBOOT_PARSE  */

			info->mem_lower_kb = 
				((struct multiboot_tag_basic_meminfo *) tag)->mem_lower;
			info->mem_upper_kb =
				((struct multiboot_tag_basic_meminfo *) tag)->mem_upper;
			break;
		case MULTIBOOT_TAG_TYPE_MMAP:

#if defined(SHOW_MULTIBOOT_PARSE)
			kprintf(KERN_DBG, "memory map:\n");
#endif  /*  SHOW_MULTIBOOT_PARSE  */

			for (mmap = ((struct multiboot_tag_mmap *) tag)->entries;
			     (multiboot_uint8_t *) mmap
				     < (multiboot_uint8_t *) tag + tag->size;
			     mmap = (multiboot_memory_map_t *)
				     ((unsigned long) mmap
					 + ((struct multiboot_tag_mmap *) tag)->entry_size)) {

#if defined(SHOW_MULTIBOOT_PARSE)
				kprintf(KERN_DBG, " base_addr = %p,"
				    " length = 0x%p, type = 0x%x\n",
				    mmap->addr, mmap->len, (unsigned) mmap->type);
#endif  /*  SHOW_MULTIBOOT_PARSE  */


				if ( mmap->type != MULTIBOOT_MEMORY_AVAILABLE ) {

					if ( info->nr_resv == HAL_MAX_RESERVED_AREA )
						continue;

					resv = &info->resv_area[info->nr_resv];
					resv->type = mmap->type;
					resv->start = mmap->addr;
					resv->end = mmap->addr + mmap->len;
					++info->nr_resv;	
				} else {

					if ( info->nr_ram == HAL_MAX_RAM_AREA )
						continue;

					ram = &info->ram_area[info->nr_ram];
					ram->type = mmap->type;
					ram->start = mmap->addr;
					ram->end = mmap->addr + mmap->len;
					++info->nr_ram;	
				}
			}
			break;
		default:
			break;
		}
	}

	return;	

error_out:
	return;	
}
