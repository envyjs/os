OUTPUT_FORMAT(elf32-i386)
ENTRY(payload_start)
SECTIONS {
    . = 0x9000;
    .entry : { *(.entry) }
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    ASSERT(. <= 0x38000, "Boot code exceeds stage-two memory budget")
    . = 0x100000;
    . = ALIGN(16);
    __bss_start = .;
    .bss (NOLOAD) : { *(.bss*) *(COMMON) }
    __bss_end = .;
    ASSERT(__bss_end <= 0xf800000, "Boot BSS exceeds 248 MiB memory budget (256 MiB RAM required)")
    /DISCARD/ : { *(.comment) *(.eh_frame*) *(.note*) }
}
