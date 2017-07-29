/* 
 * Linker script to generate an ELF file
 * that has to be converted to PS-X EXE.
 */

TARGET("elf32-littlemips")
OUTPUT_ARCH("mips")

ENTRY("main")

SECTIONS
{
	. = 0x00000000;

	__text_start = .;
	.text ALIGN(4) : { *(.text*) }
	__text_end = .;

	__rodata_start = .;
	.rodata ALIGN(4) : { *(.rodata) }
	__rodata_end = .;

	__data_start = .;
	.data ALIGN(4) : { *(.data) }
	__data_end = .;
	
	__ctor_list = .;
	.ctors ALIGN(4) : { *(.ctors) }
	__ctor_end = .;
	
	__dtor_list = .;
	.dtors ALIGN(4) : { *(.dtors) }
	__dtor_end = .;

	__bss_start = .;
	.bss  ALIGN(4) : { *(.bss) }
	__bss_end = .;

	__scratchpad = 0x1f800000;
}

