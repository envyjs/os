void kmain(void) {
    // For now, just hang forever.
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}
