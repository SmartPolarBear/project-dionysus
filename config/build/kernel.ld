OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

mboot_load_addr = 0x00100000;

SECTIONS
{
	/* Link the kernel at this address: "." means the current address */
    /* Must be equal to KERNLINK */
	. = 0xFFFFFFFF80100000; 

	PROVIDE(begin = .);

	.text : AT(mboot_load_addr) {
		PROVIDE(text_start = .);
		*(.mboot)
		*(.text .rela.text .stub .text.* .gnu.linkonce.t.*)
	    PROVIDE(text_end = .);
	}

    /* CONSTANT(MAXPAGESIZE) equals what is given to linker by -z max-page-size=0x1000 */
	. = ALIGN(CONSTANT(MAXPAGESIZE));

    .note.gnu.build-id : {
        PROVIDE_HIDDEN(__build_id_note_start = .);
        *(.note.gnu.build-id)
        PROVIDE_HIDDEN(__build_id_note_end = .);
    } :rodata :note

	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	} :rodata

	. = ALIGN(CONSTANT(MAXPAGESIZE));

	.init_array : {
        PROVIDE(start_ctors = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)
               SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array .ctors))
        PROVIDE(end_ctors = .);

        PROVIDE(start_dtors = .);
        *(.dtor*)
        PROVIDE(end_dtors = .);

        ASSERT(ALIGNOF(.init_array) == 8 || SIZEOF(.init_array) == 0,
               ".init_array overalignment -> maybe padding gaps");
    }

	. = ALIGN(CONSTANT(MAXPAGESIZE));

	/* The data segment */
	.data : {
		PROVIDE(data_start = .);
		*(.data)
		PROVIDE(data_end = .);
	}

	. = ALIGN(CONSTANT(MAXPAGESIZE));

	.bss : {
		PROVIDE(bss_start = .);
		*(.bss)
		*(.gnu.linkonce.b.*)
		*(COMMON)
		PROVIDE(bss_end = .);
	}

	. = ALIGN(CONSTANT(MAXPAGESIZE));

	PROVIDE(end = .);

	/DISCARD/ : {
		*(.eh_frame .rela.eh_frame .note.GNU-stack)
	}
}

PHDRS
{
    code PT_LOAD FLAGS(5); /* PF_R|PF_X */
    rodata PT_LOAD FLAGS(4); /* PF_R */
    data PT_LOAD FLAGS(6); /* PF_R|PF_W */
    note PT_NOTE FLAGS(4); /* PF_R */
}

mboot_load_end = mboot_load_addr + (data_end - begin);
mboot_bss_end = mboot_load_addr + (end - begin);
mboot_entry_addr = mboot_load_addr + (mboot_entry - begin);
