#ifndef KERNEL_X86_IO_H
#define KERNEL_X86_IO_H
#include <stdint.h>
static inline uint8_t in8(uint16_t port) {
    uint8_t value; __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port)); return value;
}
static inline uint16_t in16(uint16_t port) {
    uint16_t value; __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port)); return value;
}
static inline uint32_t in32(uint16_t port) {
    uint32_t value; __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port)); return value;
}
static inline void out8(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
static inline void out16(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}
static inline void out32(uint16_t port, uint32_t value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}
/* Traditional PC I/O delay, used for ATA reset and device selection. */
static inline void io_delay(void) { out8(0x80, 0); }
#endif
