bits 16
org 0x7c00
%include "layout.inc"

    jmp 0x0000:start
start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    cld
    sti
    mov [boot_drive], dl
    ; ISO blocks are 2048 bytes; USB-HDD BIOS reads use 512-byte sectors.
    mov word [sector_scale], 1
    cmp dl, 0xe0
    jae .drive_ready
    cmp dl, 0x80
    jb .drive_ready
    mov word [sector_scale], 4
.drive_ready:
    mov bx, 0x55aa
    mov ah, 0x41
    int 0x13
    jc failed
    cmp bx, 0xaa55
    jne failed
    test cx, 1
    jz failed
.block:
    mov byte [attempts], 3
.read:
    mov ax, [sector_scale]
    mov [dap + 2], ax
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jnc .advance
    xor ah, ah
    mov dl, [boot_drive]
    int 0x13
    dec byte [attempts]
    jnz .read
    jmp failed
.advance:
    add word [dap + 6], 0x80
    mov ax, [sector_scale]
    add word [dap + 8], ax
    dec word [remaining]
    jnz .block
    jmp loaded
failed:
    mov si, error_message
.print:
    lodsb
    test al, al
    jz .halt
    mov ah, 0x0e
    mov bx, 0x0007
    int 0x10
    jmp .print
.halt:
    cli
    hlt
    jmp .halt
loaded:
    mov dl, [boot_drive]
    jmp 0x0000:STAGE2_ADDRESS

align 4
dap:
    db 16, 0
    dw 1
    dw 0, STAGE2_ADDRESS >> 4
    dq STAGE2_LBA

boot_drive db 0
attempts db 0
sector_scale dw 1
; Only the loader itself belongs in conventional memory. It loads the payload
; into extended memory one sector at a time.
remaining dw 2
error_message db 'Boot read failed (BIOS EDD required)', 0
times 510-($-$$) db 0
dw 0xaa55
times 2048-($-$$) db 0
