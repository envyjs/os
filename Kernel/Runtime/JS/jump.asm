bits 32
section .text
global envy_setjmp, envy_longjmp
envy_setjmp:
    mov edx, [esp+4]
    mov [edx], ebx
    mov [edx+4], esi
    mov [edx+8], edi
    mov [edx+12], ebp
    lea eax, [esp+4]
    mov [edx+16], eax
    mov eax, [esp]
    mov [edx+20], eax
    xor eax, eax
    ret
envy_longjmp:
    mov edx, [esp+4]
    mov eax, [esp+8]
    test eax, eax
    jnz .restore
    inc eax
.restore:
    mov ebx, [edx]
    mov esi, [edx+4]
    mov edi, [edx+8]
    mov ebp, [edx+12]
    mov esp, [edx+16]
    jmp [edx+20]
