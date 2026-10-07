OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
    . = 0x00000000;
    .text : {
        KEEP(*(.text.start))
        *(.text .text.*)
    }

    . = 0x10000000;
    .rodata : {
        *(.rodata .rodata.*)
    }

    . = ALIGN(4);
    .data : {
        __global_pointer$ = . + 0x800;
        *(.data .data.*)
        *(.sdata .sdata.*)
    }

    . = ALIGN(16);
    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
        . = ALIGN(16);
        . += 4096;
        __stack_top = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}

ASSERT(SIZEOF(.data) + SIZEOF(.bss) + SIZEOF(.rodata) <= 131072,
       "static data exceeds 128 KiB")
