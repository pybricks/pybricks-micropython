/* tary, 1:09 2012/12/23 */
OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SEARCH_DIR("=/usr/local/lib");
SEARCH_DIR("=/lib");
SEARCH_DIR("=/usr/lib");
SECTIONS {
	. = 0xc0008000;
	. = ALIGN(0x4);
	PROVIDE (__text_start = .);
	.text : {
		*(.init);
		*(.text);
		. = ALIGN(0x10);

		*(.rodata .rodata.*);
		. = ALIGN(0x10);

		PROVIDE (__lds_got_start = .);
		*(.got.plt);
		*(.got);
		. = ALIGN(0x10);
		PROVIDE (__lds_got_end = .);
		PROVIDE (__lds_got_size = __lds_got_end - __lds_got_start);
		. = ALIGN(0x10);
	} = 0
	PROVIDE (__text_end = .);

	PROVIDE (__data_start = .);
	.data : {
		*(.data);
		. = ALIGN(0x10);

		*(.data.rel .data.rel.*);
		. = ALIGN(0x10);

		PROVIDE (__lds_rel_start = .);
		*(.rel.*);
		. = ALIGN(0x10);
		PROVIDE (__lds_rel_end = .);
		PROVIDE (__lds_rel_size = __lds_rel_end - __lds_rel_start);
	} = 0
	. = ALIGN(0x10);
	PROVIDE (__data_end = .);

	/*
	.rel __data_end : {
	} = 0
	*/

	.bss __lds_rel_start (NOLOAD): {
		PROVIDE (__lds_bss_start = .);
		*(.bss)
		*(COMMON)
		. = ALIGN(0x10);
		PROVIDE (__lds_bss_end = .);
	} = 0

	. = ALIGN(0x10);
	.stack : {
		*(.stack)
		. = ALIGN(0x10);
	}
	PROVIDE (end = .);

	/DISCARD/ : {
		*(.note.GNU-stack)
		*(.gnu_debuglink)
		*(.gnu.lto_*)
		/*(.debug*)*/
		*(.ARM.attributes)
		*(.comment)
		*(.interp)
		*(.dyn*)
		*(.hash)
	}
}
