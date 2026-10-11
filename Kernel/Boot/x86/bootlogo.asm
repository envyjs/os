bits 32
section .rodata
global bootlogo_start
global bootlogo_end
bootlogo_start:
    incbin "../../Assets/bootlogo.png"
bootlogo_end:
