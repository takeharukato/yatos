/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Boot time ACPI relevant routines                                  */
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

#include <hal/prepare.h>
#include <hal/kernlayout.h>
#include <hal/boot-acpi.h>

//#define DEBUG_BOOT_ACPI

static struct acpi_rdsp *
scan_rdsp(uintptr_t base, size_t len) {
	uint8_t          *p;
	unsigned int sum, n;

	for (p = (uint8_t *)PHY_TO_KERN_STRAIGHT(base); len >= sizeof(struct acpi_rdsp); len -= 4, p += 4) {

		if (memcmp(p, SIG_RDSP, 8) == 0) {

			for (sum = 0, n = 0; n < 20; n++)
				sum += p[n];
			if ((sum & 0xff) == 0)
				return (struct acpi_rdsp *) p;
		}
	}

	return (struct acpi_rdsp *) 0;  
}

static struct acpi_rdsp *
find_rdsp(void) {
	struct acpi_rdsp *rdsp;
	uintptr_t           pa;

	pa = *((uint16_t *) PHY_TO_KERN_STRAIGHT((0x40E))) << 4; // EBDA
	if (pa && (rdsp = scan_rdsp(pa, 1024)))
		return rdsp;

	return scan_rdsp(0xE0000, 0x20000);
} 

int
boot_acpiinit(karch_info *info) {
	unsigned int                               n, count;
	struct acpi_rdsp                              *rdsp;
	struct acpi_rsdt                              *rsdt;
	struct acpi_madt                              *madt;
	struct acpi_desc_header                        *hdr;
#if defined(DEBUG_BOOT_ACPI)
	unsigned char sig[5], id[7], tableid[9], creator[5];
#endif  /*  DEBUG_BOOT_ACPI  */


	madt = NULL;
	rdsp = find_rdsp();

	rsdt = (struct acpi_rsdt *)PHY_TO_KERN_STRAIGHT(rdsp->rsdt_addr_phys);
	count = (rsdt->header.length - sizeof(*rsdt)) / 4;
	for (n = 0; n < count; n++) {

		hdr = (struct acpi_desc_header *)PHY_TO_KERN_STRAIGHT(rsdt->entry[n]);

#if defined(DEBUG_BOOT_ACPI)
		memmove(sig, hdr->signature, 4); sig[4] = 0;
		memmove(id, hdr->oem_id, 6); id[6] = 0;
		memmove(tableid, hdr->oem_tableid, 8); tableid[8] = 0;
		memmove(creator, hdr->creator_id, 4); creator[4] = 0;

		kprintf(KERN_DBG, "boot-acpi: sig=%s id=%s tableid=%s oem-version=%x creator=%s creator_version:%x\n",
			sig, id, tableid, hdr->oem_revision,
			creator, hdr->creator_revision);
#endif  /*  DEBUG_BOOT_ACPI  */

		if ( !memcmp(hdr->signature, SIG_MADT, 4) ) {

			madt = (void*) hdr;
			goto success_out;
		}
	}

	kprintf(KERN_ERR, "boot-acpi: tables above 0x%x not mapped.\n", KERN_PHY_MAX );	
	return -1;

success_out:
	info->rdsp = rdsp;
	info->rsdt = rsdt;
	info->madt = madt;

#if defined(DEBUG_BOOT_ACPI)
	kprintf(KERN_INF, "boot-acpi: found rdsp: %p rsdt: %p madt: %p\n", 
		info->rdsp, info->rsdt, info->madt);
#endif  /*  DEBUG_BOOT_ACPI  */

	return 0;
}
