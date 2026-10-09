; Minimal ISO 9660 level-1 image with a BIOS El Torito no-emulation entry.
; Invoked by mkimage.bat from Kernel/Boot/x86; paths are NASM string macros.
bits 32
%include "layout.inc"
%ifdef ENVY_TREE
%include ENVY_TREE
%endif
%macro be16 1
    dw (((%1) & 0xff) << 8) | (((%1) >> 8) & 0xff)
%endmacro
%macro be32 1
    dd (((%1) & 0xff) << 24) | (((%1) & 0xff00) << 8) | (((%1) >> 8) & 0xff00) | (((%1) >> 24) & 0xff)
%endmacro
%macro both16 1
    dw %1
    be16 %1
%endmacro
%macro both32 1
    dd %1
    be32 %1
%endmacro
%macro directory_record 4
%%start:
    db %%end - %%start, 0
    both32 %1
    both32 %2
    db 126, 10, 9, 0, 0, 0, 0
    db %3, 0, 0
    both16 1
    db %%id_end - %%id
%%id: db %4
%%id_end:
    times ((%%id_end - %%id + 1) & 1) db 0
%%end:
%endmacro
%macro block 1
    times (%1)*2048-($-$$) db 0
%endmacro

block 16
pvd:
    db 1, 'CD001', 1, 0
    db 'FROM_SCRATCH_KERNEL'
    times 40-($-pvd) db ' '
    db 'KERNEL_CDFS'
    times 72-($-pvd) db ' '
    times 80-($-pvd) db 0
    both32 ISO_BLOCKS
    times 120-($-pvd) db 0
    both16 1
    both16 1
    both16 2048
%ifdef ENVY_TREE
    both32 PATH_BYTES
    dd L_TABLE, 0
    be32 M_TABLE
%else
    both32 22
    dd 20, 0
    be32 21
%endif
    dd 0
    directory_record 22, 2048, 2, 0
    times 813-($-pvd) db ' '
    times 4 db '2026100900000000', 0
    db 1, 0
block 17
boot_record:
    db 0, 'CD001', 1, 'EL TORITO SPECIFICATION'
    times 39-($-boot_record) db 0
    times 71-($-boot_record) db 0
    dd 19
block 18
    db 255, 'CD001', 1
block 19
    ; Validation entry: words sum to zero; x86 platform, 55 AA signature.
    db 1, 0
    dw 0
    times 24 db 0
    dw 0x55aa
    db 0x55, 0xaa
    ; BIOS loads four 512-byte virtual sectors at 0000:7c00.
    db 0x88, 0
    dw 0
    db 0, 0
    dw 4
    dd BOOT_LBA
    times 20 db 0
block 20
    db 1, 0
    dd 22
    dw 1
    db 0, 0
    db 4, 0
    dd 23
    dw 1
    db 'DOCS'
block 21
    db 1, 0
    be32 22
    be16 1
    db 0, 0
    db 4, 0
    be32 23
    be16 1
    db 'DOCS'
block 22
    directory_record 22, 2048, 2, 0
    directory_record 22, 2048, 2, 1
    directory_record BOOT_LBA, (STAGE2_SECTORS+1)*2048, 0, 'BOOT.BIN;1'
    directory_record 19, 2048, 0, 'BOOT.CAT;1'
    directory_record 23, 2048, 2, 'DOCS'
%ifdef ENVY_TREE
    directory_record ENVY_LBA, ENVY_SIZE, 2, 'ENVY'
%endif
block 23
    directory_record 23, 2048, 2, 0
    directory_record 22, 2048, 2, 1
    directory_record README_LBA, file_end-file_start, 0, 'README.TXT;1'
block BOOT_LBA
    incbin STAGE1
block STAGE2_LBA
stage2_start:
    incbin STAGE2
%if ($-stage2_start) != STAGE2_SECTORS*2048
    %error "Second-stage size does not match STAGE2_SECTORS in layout.inc"
%endif
block README_LBA
file_start:
    incbin README
file_end:
%ifdef ENVY_TREE
block 128
envy_body
%else
block ISO_BLOCKS
%endif
