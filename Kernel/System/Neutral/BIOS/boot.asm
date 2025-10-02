mov ah, 0x0e ; tty mode
mov al, 'E'
int 0x10
mov al, 'n'
int 0x10
mov al, 'v'
int 0x10
mov al, 'y'
int 0x10
mov al, ' '
int 0x10
mov al, 'K'
int 0x10
mov al, 'e'
int 0x10
mov al, 'r'
int 0x10
mov al, 'n'
int 0x10
mov al, 'e'
int 0x10
mov al, 'l'
int 0x10

jmp $ ; jump to current address = infinite loop

; padding and magic number
times 510 - ($-$$) db 0
dw 0xaa55 