bits 32
section .entry
global payload_start
extern boot_main, __bss_start, __bss_end
payload_start:
    ; ES5 Number and Math operations use the x87 FPU.
    fninit
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb
    and esp, -16
    call boot_main
.halt:
    cli
    hlt
    jmp .halt
