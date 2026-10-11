bits 32
section .text
global boot_splash_irq_stub
extern framebuffer_boot_splash_irq

boot_splash_irq_stub:
    pushad
    cld
    call framebuffer_boot_splash_irq
    mov al, 0x20
    out 0x20, al
    popad
    iretd
