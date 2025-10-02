org 0x7c00                ; Set origin to boot sector load address

mov si, msg              ; SI points to message

print_loop:
    mov ah, 0x0e         ; BIOS teletype function
    lodsb                ; Load byte at DS:SI into AL, increment SI
    cmp al, 0            ; Check for null terminator
    je done_print
    int 0x10             ; Print character in AL
    jmp print_loop

done_print:
    jmp $                ; Infinite loop

msg: db 'Starting Envy...', 0

times 510 - ($ - $$) db 0
dw 0xaa55               ; Boot sector signature
