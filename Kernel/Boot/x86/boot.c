#include "../../FS/CDFS/cdfs.h"
#include "../../Drivers/Storage/atapi.h"
#include "../../Arch/x86/io.h"
#include "../../Browser/browser.h"
#include "../../Drivers/Video/framebuffer.h"
#include "../../Drivers/Input/ps2.h"
static cdfs filesystem;
static atapi_device devices[ATAPI_MAX_DEVICES];
static uint32_t cursor;
static void print(const char *text) {
    if(framebuffer_get()) {
#ifdef BOOT_TESTS
        for(const char *p=text;*p;p++) out8(0xe9,(uint8_t)*p);
#endif
        framebuffer_console_write(text);return;
    }
    volatile uint16_t *screen = (volatile uint16_t *)0xb8000;
    while (*text && cursor < 2000) {
#ifdef BOOT_TESTS
        out8(0xe9, (uint8_t)*text);
#endif
        if (*text == '\n') cursor = (cursor / 80 + 1) * 80;
        else if (*text != '\r') screen[cursor++] = 0x0f00 | (uint8_t)*text;
        ++text;
    }
}
#ifdef BOOT_TESTS
static void test_exit(uint32_t result) { out32(0xf4, result); }
#endif
#ifdef BOOT_FONT_TESTS
static void font_test_exit(uint32_t result) { out32(0xf4,result); }
#endif
#ifdef BOOT_IMAGE_TESTS
static void image_test_exit(uint32_t result) { out32(0xf4,result); }
#endif
void boot_main(void) {
    framebuffer_init();
    print("Bootloader ready: x86-32 protected mode\n");
    uint32_t count = atapi_probe(devices, ATAPI_MAX_DEVICES);
    atapi_device *cd = 0;
    for (uint32_t i = 0; i < count; ++i) {
        int ready = atapi_media_ready(&devices[i]);
        if (ready) continue;
        if (!cdfs_mount(&filesystem, atapi_read_block, &devices[i])) { cd = &devices[i]; break; }
    }
    if (!cd) {
        print(count ? "No readable CDFS CD found\n" : "No supported IDE ATAPI CD drive found\n");
#ifdef BOOT_TESTS
        test_exit(count ? 99 : 100);
#endif
        return;
    }
    print("Native ATAPI PIO: "); print(cd->model); print("\n");
#ifdef BOOT_IMAGE_TESTS
    extern int image_self_test(cdfs *fs);
    int image_failure=image_self_test(&filesystem);
    const char *image_message=image_failure?"Image tests FAILED: ":"Image tests PASSED: ";
    for(const char *p=image_message;*p;p++)out8(0xe9,(uint8_t)*p);
    const char *digits="0123456789abcdef";for(int i=28;i>=0;i-=4)out8(0xe9,(uint8_t)digits[((uint32_t)image_failure>>(uint32_t)i)&15]);out8(0xe9,'\n');
    image_test_exit(image_failure?(uint32_t)image_failure:16);return;
#endif
#ifdef BOOT_FONT_TESTS
    extern int font_self_test(cdfs *fs);
    int font_failure=font_self_test(&filesystem);
    const char *message=font_failure?"Font tests FAILED\n":"Font tests PASSED\n";
    for(const char *p=message;*p;p++)out8(0xe9,(uint8_t)*p);
    font_test_exit(font_failure?(uint32_t)font_failure:16);return;
#endif
#ifdef BOOT_TESTS
    extern int atapi_self_test(atapi_device *device, cdfs *fs);
    extern int cdfs_self_test(cdfs *fs, atapi_device *device);
    extern int js_self_test(void);
    extern int css_self_test(void);
    extern int input_self_test(void);
    int failure = atapi_self_test(cd, &filesystem);
    if (!failure) failure = cdfs_self_test(&filesystem, cd);
    if (!failure) failure = js_self_test();
    if (!failure) failure = css_self_test();
    if (!failure) failure = input_self_test();
    print(failure ? "ATAPI/CDFS/JS/CSS tests FAILED\n" : "ATAPI/CDFS/JS/CSS tests PASSED\n");
    uint32_t result = failure ? (uint32_t)failure : 16;
    if(failure){const char *digits="0123456789abcdef";for(int i=28;i>=0;i-=4)out8(0xe9,(uint8_t)digits[(result>>(uint32_t)i)&15]);out8(0xe9,'\n');}
    test_exit(result);
    return;
#endif
    print("CDFS mounted: ISO 9660\n");
    const framebuffer *fb=framebuffer_get();input_init(fb?fb->width:800,fb?fb->height:600);
    browser_start(&filesystem, print);
    /* Poll with interrupts disabled until an IDT and IRQ handlers exist. */
    for(;;) {
        input_poll();input_event event;while(input_next(&event))browser_input(&event);
        browser_poll();
        __asm__ volatile ("pause");
    }
}
