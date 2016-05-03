OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

BOOT_STACK_SIZE = 0x2000; /*  kern/param.hのKSTACK_SIZEと同じ値に設定  */
KERN_VMA_BASE = 0xFFFF800000000000;
KERN_BASE = 0x100000;
ENTRY(start)
SECTIONS
{
    . = KERN_BASE;
    _kernel_start = .;
    .boot : {
        *(.boot_text)
        *(.boot_data)
	. = ALIGN(BOOT_STACK_SIZE);
	_bsp_stack_start = .;
	bsp_stack = .;
	. += BOOT_STACK_SIZE;
	_bsp_stack_end = .;
    }

    . += KERN_VMA_BASE;
    .text : AT(ADDR(.text) - KERN_VMA_BASE) {
        *(.text .rela.text .stub .text.* .gnu.linkonce.t.*)
        _rodata_start = .;
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        _rodata_end = .;
    }

    .data : AT(ADDR(.data) - KERN_VMA_BASE) {
        _data_start = .;
        *(.data)
        _data_end = .;
    }

    . = ALIGN(0x1000);
    .bss :  AT(ADDR(.bss) - KERN_VMA_BASE) {
        _bss_start = .;
        *(.bss)
	*(COMMON)
        _bss_end = .;
    }
    . = ALIGN(0x1000);
    _kernel_end = . - KERN_VMA_BASE;

    /DISCARD/ :
    {
	         *(.gcc_except_table)
                 *(.eh_frame)
                 *(.note)
                 *(.comment)
                 *(.rel.*)
                 *(.rela.*)
    }
}

