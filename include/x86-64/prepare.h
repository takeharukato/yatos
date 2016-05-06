/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  preparation relevant definitions                                  */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_PREPARE_H)
#define  _HAL_PREPARE_H 
#include <stddef.h>
#include <stdint.h>

#include <kern/config.h>

#if defined(CONFIG_HAL_MAX_MB_MODULES)
#define HAL_MAX_MB_MOD (CONFIG_HAL_MAX_MB_MODULES)
#else
#define HAL_MAX_MB_MOD 32
#endif  /*  CONFIG_HAL_MAX_MB_MODULES  */

#if defined(CONFIG_HAL_MAX_KPARAM_LEN)
#define HAL_MB_PARAM_LEN (CONFIG_HAL_MAX_KPARAM_LEN)
#else
#define HAL_MB_PARAM_LEN (128)
#endif  /*  ONFIG_HAL_MAX_KPARAM_LEN  */

#define HAL_MAX_RESERVED_AREA (32)

#define HAL_MAX_RAM_AREA       (3)

typedef struct _grub_mod{
	char      param[HAL_MB_PARAM_LEN];
	uintptr_t                   start;
	uintptr_t                     end;
}grub_mod;

typedef struct _memory_area{
	uint32_t   type;
	uintptr_t start;
	uintptr_t   end;
}memory_area;

/** OSのブート時に必要な情報
 */
struct acpi_rdsp;
struct acpi_rsdt;
struct acpi_madt;
typedef struct _karch_info{
	char                  kparam[HAL_MB_PARAM_LEN];
	int                                     nr_mod;
	int                                    nr_resv;
	int                                     nr_ram;
	uint64_t                          mem_lower_kb;
	uint64_t                          mem_upper_kb;
	uint64_t                           phy_mem_min;
	uint64_t                           phy_mem_max;
	grub_mod               modules[HAL_MAX_MB_MOD];
	memory_area         ram_area[HAL_MAX_RAM_AREA];
	memory_area   resv_area[HAL_MAX_RESERVED_AREA];
	uintptr_t                boot_kpgtbl_start_phy;
	void                                   *kpgtbl;
	void                              *boot_kpgtbl;
	struct acpi_rdsp                         *rdsp;
	struct acpi_rsdt                         *rsdt;
	struct acpi_madt                         *madt;
}karch_info;

struct _page_frame_info;
void x86_64_boot_map_kernel(karch_info *_info);
void x86_64_parse_multiboot2_info(uint64_t _magic, uint64_t _mbaddr, karch_info *_info);
void x86_64_alloc_page_info(uintptr_t _min_paddr, uintptr_t _max_paddr);
void x86_64_remap_kernel(karch_info *_info);
void x86_64_release_boot_reserved_pages(karch_info *_info);
karch_info  *_refer_boot_info(void);
#endif  /*  _HAL_PREPARE_H   */
