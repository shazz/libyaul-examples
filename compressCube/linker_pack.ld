/* Sega Saturn Linker Script by Anders Montonen		*/
/* based on Jeff Frohwein's GBA script				*/
/*  v0.1 - First release							*/
/*													*/
/* This file is released into the public domain		*/
/* for any use without restrictions placed			*/
/* upon it.											*/

/* This linker script sets up code to run from		*/
/* 0x000210000	
 */

/* OUTPUT_FORMAT("elf32-sh") */
OUTPUT_ARCH(sh)
ENTRY(_start)

__text_start = 	0x000210000;
__stack_bottom = 	0x0002F7FFF;
__stack_top	= 	0x0002FFFFF;
_stack = __stack_top;

SECTIONS
{
	.text __text_start :
	{
		*(.text.*)
		*(.gnu.warning)
		*(.gnu.linkonce.t*)
		. = ALIGN(2);
	} = 0
	__text_end = .;
	
	.bss ALIGN(2) :
	{
		__bss_start = ABSOLUTE(.);
		__bss_start__ = ABSOLUTE(.);
		*(.dynbss)
		*(.gnu.linkonce.b*)
		*(COMMON)
		. = ALIGN(2);
	}
	
	__bss_end = .;
	__bss_end__ = .;

	.stack __stack_bottom : { _stack = .; *(.stack) }

	.stack __stack_bottom :
	{
		*(.stack)
		. = ALIGN(2);
	} = 0xdeaddead
	
	_end = .;
	__end__ = _end;
	PROVIDE (end = _end);
	
	/* Stabs debugging sections.  */
	.stab 0 : { *(.stab) }
	.stabstr 0 : { *(.stabstr) }
	.stab.excl 0 : { *(.stab.excl) }
	.stab.exclstr 0 : { *(.stab.exclstr) }
	.stab.index 0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }
	.comment 0 : { *(.comment) }
	/* DWARF debug sections. */
	/* Symbols in the DWARF debugging sections are relative to the beginning */
	/* of the section so we begin them at 0.  */
	/* DWARF 1 */
	.debug          0 : { *(.debug) }
	.line           0 : { *(.line) }
	/* GNU DWARF 1 extensions */
	.debug_srcinfo  0 : { *(.debug_srcinfo) }
	.debug_sfnames  0 : { *(.debug_sfnames) }
	/* DWARF 1.1 and DWARF 2 */
	.debug_aranges  0 : { *(.debug_aranges) }
	.debug_pubnames 0 : { *(.debug_pubnames) }
	/* DWARF 2 */
	.debug_info     0 : { *(.debug_info) }
	.debug_abbrev   0 : { *(.debug_abbrev) }
	.debug_line     0 : { *(.debug_line) }
	.debug_frame    0 : { *(.debug_frame) }
	.debug_str      0 : { *(.debug_str) }
	.debug_loc      0 : { *(.debug_loc) }
	.debug_macinfo  0 : { *(.debug_macinfo) }

	/* These must appear regardless of  .  */
}

