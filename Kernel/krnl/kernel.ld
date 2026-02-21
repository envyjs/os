ENTRY(kmain)

SECTIONS
{
    . = 0x100000;  /* Load kernel at 1 MiB */

    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
        *(.bss*)
    }
}
