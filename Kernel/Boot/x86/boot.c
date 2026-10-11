#include "../../FS/CDFS/cdfs.h"
#include "../../Drivers/Storage/atapi.h"
#include "../../Arch/x86/io.h"
#include "../../Browser/browser.h"
#include "../../Drivers/Video/framebuffer.h"
#include "../../Drivers/Input/ps2.h"
#include "../../Drivers/Storage/sata.h"
#include "../../Drivers/USB/usb.h"
#include "../../FS/FAT32/fat32.h"
static sata_device sata_disks[SATA_MAX_DEVICES];
static fat32 disk_filesystems[SATA_MAX_DEVICES];
static usb_controller usb_controllers[USB_MAX_CONTROLLERS];
static fat32 usb_filesystems[USB_MAX_STORAGE];
static cdfs filesystem;
static atapi_device devices[ATAPI_MAX_DEVICES];
static uint32_t cursor;
static void print(const char *text) {
    if(framebuffer_get()) {
#ifdef BOOT_TESTS
        for(const char *p=text;*p;p++) out8(0xe9,(uint8_t)*p);
#endif
        if(framebuffer_boot_splash_update())return;
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
typedef struct {uint16_t offset_low,selector;uint8_t zero,flags;uint16_t offset_high;} __attribute__((packed)) boot_idt_gate;
typedef struct {uint16_t limit;uint32_t base;} __attribute__((packed)) boot_idtr;
static boot_idt_gate boot_idt[256];
extern void boot_splash_irq_stub(void);
static void boot_splash_timer_start(void){
    for(uint32_t i=0;i<256;i++)boot_idt[i]=(boot_idt_gate){0};
    uint32_t handler=(uint32_t)(uintptr_t)boot_splash_irq_stub;
    boot_idt[32]=(boot_idt_gate){(uint16_t)handler,0x08,0,0x8e,(uint16_t)(handler>>16)};
    boot_idtr idtr={(uint16_t)(sizeof(boot_idt)-1),(uint32_t)(uintptr_t)boot_idt};
    __asm__ volatile("lidt %0"::"m"(idtr));
    out8(0x20,0x11);out8(0xa0,0x11);out8(0x21,0x20);out8(0xa1,0x28);
    out8(0x21,0x04);out8(0xa1,0x02);out8(0x21,0x01);out8(0xa1,0x01);
    out8(0x43,0x34);out8(0x40,0xae);out8(0x40,0x4d);
    browser_set_pit_reload(19886);framebuffer_boot_splash_irq_mode(1);out8(0xa1,0xff);out8(0x21,0xfe);
    __asm__ volatile("sti":: :"memory");
}
void boot_main(void) {
    framebuffer_init();
#if !defined(BOOT_TESTS) && !defined(BOOT_FONT_TESTS) && !defined(BOOT_IMAGE_TESTS) && !defined(BOOT_STORAGE_TESTS)
    framebuffer_boot_splash_start();
    boot_splash_timer_start();
#endif
    print("Bootloader ready: x86-32 protected mode\n");
#ifdef BOOT_STORAGE_TESTS
    extern int storage_self_test(void);int storage_failure=storage_self_test();
    const char *storage_message=storage_failure?"Storage tests FAILED: ":"Storage tests PASSED: ";for(const char *p=storage_message;*p;p++)out8(0xe9,(uint8_t)*p);
    const char *storage_digits="0123456789abcdef";for(int i=28;i>=0;i-=4)out8(0xe9,(uint8_t)storage_digits[((uint32_t)storage_failure>>(uint32_t)i)&15]);out8(0xe9,'\n');out32(0xf4,storage_failure?(uint32_t)storage_failure:16);return;
#endif
    uint32_t disk_count=sata_probe(sata_disks,SATA_MAX_DEVICES);
    framebuffer_boot_splash_update();
    for(uint32_t i=0;i<disk_count;i++){print("SATA AHCI: ");print(sata_disks[i].model);print("\n");if(!fat32_mount(&disk_filesystems[i],sata_read,sata_write,sata_flush,&sata_disks[i],sata_disks[i].sectors))print("FAT32 volume mounted\n");}
    uint32_t usb_count=usb_init(usb_controllers,USB_MAX_CONTROLLERS);
    framebuffer_boot_splash_update();
    for(uint32_t i=0;i<usb_count;i++){
        print("USB host: ");print(usb_controller_name(usb_controllers[i].kind));
        print(usb_controllers[i].connected_ports?" (device attached)\n":" (no device attached)\n");
    }
    for(uint32_t i=0;i<usb_storage_count();i++){
        usb_msc_device *disk=usb_storage_get(i);if(!disk)continue;
        if(!fat32_mount(&usb_filesystems[i],usb_msc_read,usb_msc_write,usb_msc_flush,disk,disk->sectors))print("USB Mass Storage FAT32 volume mounted\n");
    }
    uint32_t count = atapi_probe(devices, ATAPI_MAX_DEVICES);
    framebuffer_boot_splash_update();
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
    __asm__ volatile("cli":: :"memory");out8(0x21,0xff);out8(0xa1,0xff);
    framebuffer_boot_splash_stop();
    /* Poll with interrupts disabled until an IDT and IRQ handlers exist. */
    for(;;) {
        input_poll();usb_poll();input_event event,pending;int have_pending=0;
        while(input_next(&event)){
            if(have_pending&&pending.type==INPUT_POINTER&&event.type==INPUT_POINTER&&pending.buttons==event.buttons&&
                pending.modifiers==event.modifiers&&!pending.wheel&&!event.wheel){
                pending.dx+=event.dx;pending.dy+=event.dy;pending.x=event.x;pending.y=event.y;continue;
            }
            if(have_pending){browser_input(&pending);have_pending=0;}
            if(event.type==INPUT_POINTER&&!event.wheel){pending=event;have_pending=1;}
            else browser_input(&event);
        }
        if(have_pending)browser_input(&pending);
        browser_poll();
        __asm__ volatile ("pause");
    }
}
