/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Boot time ACPI definitions                                        */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_BOOT_ACPI_H)
#define  _HAL_BOOT_ACPI_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>

#define SIG_RDSP "RSD PTR "
#define SIG_MADT "APIC"

#define BOOT_ACPI_TYPE_LAPIC            (0)
#define BOOT_ACPI_TYPE_IOAPIC           (1)
#define BOOT_ACPI_TYPE_INT_SRC_OVERRIDE (2)
#define BOOT_ACPI_TYPE_NMI_INT_SRC      (3)
#define BOOT_ACPI_TYPE_LAPIC_NMI        (4)

#define BOOT_ACPI_APIC_LAPIC_ENABLED    (1)

struct acpi_rdsp {
	uint8_t signature[8];
	uint8_t checksum;
	uint8_t oem_id[6];
	uint8_t revision;
	uint32_t rsdt_addr_phys;
	uint32_t length;
	uint64_t xsdt_addr_phys;
	uint8_t xchecksum;
	uint8_t reserved[3];
} __attribute__((__packed__));

struct acpi_desc_header {
	uint8_t signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	uint8_t oem_id[6];
	uint8_t oem_tableid[8];
	uint32_t oem_revision;
	uint8_t creator_id[4];
	uint32_t creator_revision;
} __attribute__((__packed__));

struct acpi_rsdt {
	struct acpi_desc_header header;
	uint32_t entry[0];
} __attribute__((__packed__));

struct acpi_madt {
	struct acpi_desc_header header;
	uint32_t lapic_addr_phys;
	uint32_t flags;
	uint8_t table[0];
} __attribute__((__packed__));

struct madt_lapic {
  uint8_t type;
  uint8_t length;
  uint8_t acpi_id;
  uint8_t apic_id;
  uint32_t flags;
} __attribute__((__packed__));

struct madt_ioapic {
  uint8_t type;
  uint8_t length;
  uint8_t id;
  uint8_t reserved;
  uint32_t addr;
  uint32_t interrupt_base;
} __attribute__((__packed__));

struct _karch_info;
int boot_acpiinit(struct _karch_info *info);

#endif  /*  _HAL_BOOT_ACPI_H   */
