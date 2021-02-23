OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(start)

SECTIONS
{
	/* Link the kernel at this address: "." means the curproc address */
    /* Must be equal to KERNLINK */
	. = 0x7000; 

	.text : AT(0x7000) {
		*(.text .rela.text .stub .text.* .gnu.linkonce.t.*)
	}


	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	/* Adjust the address for the data segment to the next page */
	. = ALIGN(0x1000);

	/* The data segment */
	.data : {
        KEEP(*( .init_array ));
        KEEP(*(SORT_BY_INIT_PRIORITY( .init_array.* )));
        *(.ctor*)
        *(.dtor*)
		*(.data)
	}

	. = ALIGN(0x1000);


	.bss : {
		*(.bss)
		*(COMMON)
	}

	. = ALIGN(0x1000);


	/DISCARD/ : {
		*(.eh_frame .rela.eh_frame .note.GNU-stack .note.gnu.build-id)
	}
}
