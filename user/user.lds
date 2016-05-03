OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

USER_VMA_BASE = 0x400000;
ENTRY(_start)
SECTIONS
{
    . = USER_VMA_BASE;
     _start = .;
     _text = .;			/* Text and read-only data */
     .text : {
	*(.text .rela.text .stub .text.* .gnu.linkonce.t.*)
	*(.gnu.warning)
     }
     .rodata : { *(.rodata) }
     _etext = .;			/* End of text section */
    . = ALIGN(0x1000);
    .data : {			/* Data */
	*(.data)
	CONSTRUCTORS
     }
     _edata = .;			/* End of data section */
    . = ALIGN(0x1000);
    .bss :  {
        _bss_start = .;
        *(.bss)
	*(COMMON)
        _bss_end = .;
    }
    . = ALIGN(0x1000);
    _user_end = .;

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

