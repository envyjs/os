bits 16
org 0x8000
%include "layout.inc"

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    cld
    mov [boot_drive], dl
    ; Keep each BIOS transfer below 64 KiB for both optical and USB disks.
    mov word [sector_scale], 1
    cmp dl, 0xe0
    jae .drive_ready
    cmp dl, 0x80
    jb .drive_ready
    mov word [sector_scale], 4
.drive_ready:
    mov eax, STAGE2_LBA+2
    movzx ecx, word [sector_scale]
    imul eax, ecx
    mov [payload_dap+8], eax
    ; BIOS cannot transfer a large JavaScript engine directly above 1 MiB.
    ; Read batches below the BIOS 64 KiB transfer boundary, then copy them.
    in al, 0x92
    or al, 2
    and al, 0xfe
    out 0x92, al
    mov word [payload_remaining], STAGE2_SECTORS-2
.payload_block:
    mov ax, [payload_remaining]
    cmp ax, 31
    jbe .payload_chunk
    mov ax, 31
.payload_chunk:
    mov [payload_chunk], ax
    mov byte [payload_attempts], 3
.payload_read:
    mov ax, [payload_chunk]
    mul word [sector_scale]
    mov [payload_dap+2], ax
    mov si, payload_dap
    mov dl, [boot_drive]
    mov ah, 0x42
    sti
    int 0x13
    jnc .payload_ready
    xor ah, ah
    mov dl, [boot_drive]
    int 0x13
    dec byte [payload_attempts]
    jnz .payload_read
    cmp word [payload_chunk], 1
    je .payload_failed
    mov word [payload_chunk], 1
    mov byte [payload_attempts], 3
    jmp .payload_read
.payload_failed:
    mov si, payload_error
.payload_error:
    lodsb
    test al, al
    jz .payload_halt
    mov ah, 0x0e
    mov bx, 7
    int 0x10
    jmp .payload_error
.payload_halt:
    cli
    hlt
    jmp .payload_halt
.payload_ready:
    cli
    lgdt [gdt_pointer]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SELECTOR:payload_copy
bits 32
payload_copy:
    mov ax, DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esi, 0x10000
    mov edi, [payload_destination]
    movzx ecx, word [payload_chunk]
    shl ecx, 9
    rep movsd
    movzx eax, word [payload_chunk]
    shl eax, 11
    add [payload_destination], eax
    jmp 0x18:payload_real
bits 16
payload_real:
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov eax, cr0
    and eax, ~1
    mov cr0, eax
    jmp 0:payload_next
payload_next:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    movzx eax, word [payload_chunk]
    movzx ecx, word [sector_scale]
    imul eax, ecx
    add [payload_dap+8], eax
    sub [payload_remaining], ax
    jnz start.payload_block
    ; Select VGA text mode before leaving BIOS services.
    mov ax, 0x0003
    int 0x10
    ; Copy the BIOS 8x16 font for the protected-mode framebuffer driver.
    mov ax, 0x1130
    mov bh, 6
    int 0x10
    push ds
    mov ax, es
    mov ds, ax
    mov si, bp
    xor ax, ax
    mov es, ax
    mov di, 0x6000
    mov cx, 4096
    cld
    rep movsb
    pop ds
    ; Discover a linear 800x600 direct-color VBE mode.
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov dword [0x5400], 0
    mov word [0x54f0], 0
    mov dword [0x5000], 'VBE2'
    mov di, 0x5000
    mov ax, 0x4f00
    int 0x10
    cmp ax, 0x004f
    jne .video_done
    mov si, [0x500e]
    mov ax, [0x5010]
    mov fs, ax
    mov word [mode_budget], 512
.mode:
    mov cx, [fs:si]
    add si, 2
    cmp cx, 0xffff
    je .video_done
    mov [video_mode], cx
    push si
    push fs
    xor ax, ax
    mov es, ax
    mov di, 0x5200
    mov ax, 0x4f01
    int 0x10
    pop fs
    pop si
    cmp ax, 0x004f
    jne .next_mode
    mov ax, [0x5200]
    and ax, 0x0091
    cmp ax, 0x0091
    jne .next_mode
    cmp byte [0x521b], 6
    jne .next_mode
    cmp byte [0x5219], 32
    je .record_mode
    cmp byte [0x5219], 24
    jne .next_mode
.record_mode:
    ; Keep the BIOS-reported linear framebuffer modes for the kernel display API.
    ; Records are width/height pairs at 0x5500; cap the list below the BIOS font.
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    mov ax, [0x54f0]
    xor bx, bx
.mode_duplicate:
    cmp bx, ax
    jae .mode_append
    mov di, bx
    shl di, 2
    add di, 0x5500
    mov dx, [di]
    cmp dx, [0x5212]
    jne .mode_next_duplicate
    mov dx, [di+2]
    cmp dx, [0x5214]
    je .mode_recorded
.mode_next_duplicate:
    inc bx
    jmp .mode_duplicate
.mode_append:
    cmp ax, 128
    jae .mode_recorded
    mov di, ax
    shl di, 2
    add di, 0x5500
    mov dx, [0x5212]
    mov [di], dx
    mov dx, [0x5214]
    mov [di+2], dx
    inc ax
    mov [0x54f0], ax
.mode_recorded:
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    cmp word [0x5212], 800
    jne .next_mode
    cmp word [0x5214], 600
    jne .next_mode
.set_mode:
    mov bx, [video_mode]
    or bx, 0x4000
    mov ax, 0x4f02
    int 0x10
    cmp ax, 0x004f
    jne .video_done
    mov dword [0x5400], 0x31454256 ; "VBE1", consumed by framebuffer.c
    jmp .video_done
.next_mode:
    dec word [mode_budget]
    jnz .mode
.video_done:
    cli
    xor ax, ax
    mov ds, ax
    ; Fast A20 gate; preserve other bits and keep reset deasserted.
    in al, 0x92
    or al, 2
    and al, 0xfe
    out 0x92, al
    lgdt [gdt_pointer]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SELECTOR:protected_start

align 8
gdt:
    dq 0
    dq 0x00cf9a000000ffff
    dq 0x00cf92000000ffff
    dq 0x00009a000000ffff
    dq 0x000092000000ffff
gdt_end:
gdt_pointer:
    dw gdt_end - gdt - 1
    dd gdt

bits 32
protected_start:
    mov ax, DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    ; SVG rasterization uses x87. Clear EM/TS and initialize its stack.
    mov eax, cr0
    and eax, ~0x0c
    or eax, 2
    mov cr0, eax
    fninit
    cld
    call 0x100000
.halt:
    ; Interrupts stay disabled until a future kernel installs an IDT.
    cli
    hlt
    jmp .halt

boot_drive db 0
sector_scale dw 1
video_mode dw 0
mode_budget dw 0
payload_remaining dw 0
payload_chunk dw 0
payload_attempts db 0
payload_destination dd 0x100000
align 4
payload_dap:
    db 16, 0
    dw 1
    dw 0, 0x1000
    dq STAGE2_LBA+2
payload_error db 'Boot payload read failed', 0
times 0x1000-($-$$) db 0
embedded_payload:
incbin PAYLOAD
embedded_payload_end:
times STAGE2_SECTORS*2048-($-$$) db 0
