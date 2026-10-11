SHELL := cmd.exe
.SHELLFLAGS := /c
.DEFAULT_GOAL := all

ROOT := ../../..
OUT := $(ROOT)/Build/Boot/x86
NASM ?= nasm
CLANG ?= clang
LLD ?= ld.lld
TESTS ?= 0
FONT_TESTS ?= 0
IMAGE_TESTS ?= 0
STORAGE_TESTS ?= 0
CFLAGS := --target=i686-none-elf -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -mno-sse -mno-mmx -msoft-float -std=c11 -Wall -Wextra -Werror -Oz -ffunction-sections -fdata-sections
OBJECTS := $(OUT)/payload.o $(OUT)/bootlogo.o $(OUT)/splash_irq.o $(OUT)/boot.o $(OUT)/cdfs.o $(OUT)/atapi.o $(OUT)/memory.o $(OUT)/js.o $(OUT)/json.o $(OUT)/js_full.o $(OUT)/quickjs.o $(OUT)/qjs_cutils.o $(OUT)/qjs_dtoa.o $(OUT)/qjs_regexp.o $(OUT)/qjs_unicode.o $(OUT)/js_support.o $(OUT)/js_jump.o $(OUT)/browser.o $(OUT)/framebuffer.o $(OUT)/html.o $(OUT)/css.o $(OUT)/ps2.o $(OUT)/ttf.o $(OUT)/font.o $(OUT)/image.o $(OUT)/image_support.o $(OUT)/images.o $(OUT)/usb.o $(OUT)/uhci.o $(OUT)/ohci.o $(OUT)/ehci.o $(OUT)/xhci.o $(OUT)/usb_hid.o $(OUT)/usb_device.o $(OUT)/usb_msc.o
OBJECTS += $(OUT)/sata.o $(OUT)/fat32.o
ENVY_TREE := $(OUT)/envy.inc
IMAGE_PREVIEWS := $(OUT)/Previews
ifeq ($(TESTS),1)
ENVY_TREE :=
CFLAGS += -DBOOT_TESTS
OBJECTS += $(OUT)/test.o $(OUT)/atapi_test.o $(OUT)/js_test.o $(OUT)/css_test.o $(OUT)/input_test.o
endif
ifeq ($(FONT_TESTS),1)
IMAGE_PREVIEWS :=
CFLAGS += -DBOOT_FONT_TESTS
OBJECTS += $(OUT)/font_test.o
endif
ifeq ($(IMAGE_TESTS),1)
IMAGE_PREVIEWS :=
CFLAGS += -DBOOT_IMAGE_TESTS
OBJECTS += $(OUT)/image_test.o
endif

.PHONY: all FORCE previews
ifeq ($(STORAGE_TESTS),1)
ENVY_TREE :=
CFLAGS += -DBOOT_STORAGE_TESTS
OBJECTS += $(OUT)/storage_test.o
endif
all: $(OUT)/boot.iso

$(OUT):
	cmd.exe /c if not exist "$(subst /,\,$(OUT))" mkdir "$(subst /,\,$(OUT))"

$(OUT)/stage1.bin: stage1.asm layout.inc | $(OUT)
	$(NASM) -f bin -I ./ stage1.asm -o $@

$(OUT)/cdfs.o: ../../FS/CDFS/cdfs.c ../../FS/CDFS/cdfs.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/sata.o: ../../Drivers/Storage/sata.c ../../Drivers/Storage/sata.h ../../Arch/x86/io.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/usb.o: ../../Drivers/USB/usb.c ../../Drivers/USB/usb.h ../../Drivers/USB/hc.h ../../Drivers/USB/device.h ../../Drivers/USB/hid.h ../../Drivers/USB/msc.h ../../Drivers/Input/ps2.h ../../Arch/x86/io.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/uhci.o: ../../Drivers/USB/uhci.c ../../Drivers/USB/usb.h ../../Drivers/USB/hc.h ../../Arch/x86/io.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/ohci.o: ../../Drivers/USB/ohci.c ../../Drivers/USB/usb.h ../../Drivers/USB/hc.h ../../Arch/x86/io.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/ehci.o: ../../Drivers/USB/ehci.c ../../Drivers/USB/usb.h ../../Drivers/USB/hc.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/xhci.o: ../../Drivers/USB/xhci.c ../../Drivers/USB/usb.h ../../Drivers/USB/hc.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/usb_hid.o: ../../Drivers/USB/hid.c ../../Drivers/USB/hid.h ../../Drivers/Input/ps2.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/usb_device.o: ../../Drivers/USB/device.c ../../Drivers/USB/device.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/usb_msc.o: ../../Drivers/USB/msc.c ../../Drivers/USB/msc.h ../../Drivers/USB/device.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/fat32.o: ../../FS/FAT32/fat32.c ../../FS/FAT32/fat32.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/storage_test.o: ../../Tests/Storage/test.c ../../Drivers/Storage/sata.h ../../FS/FAT32/fat32.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/boot.o: boot.c ../../FS/CDFS/cdfs.h ../../Drivers/Storage/atapi.h ../../Drivers/Storage/sata.h ../../FS/FAT32/fat32.h ../../Drivers/USB/usb.h ../../Drivers/USB/msc.h ../../Arch/x86/io.h ../../Browser/browser.h ../../Drivers/Video/framebuffer.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/payload.o: payload.asm | $(OUT)
	$(NASM) -f elf32 $< -o $@

$(OUT)/bootlogo.o: bootlogo.asm ../../Assets/bootlogo.png | $(OUT)
	$(NASM) -f elf32 $< -o $@

$(OUT)/splash_irq.o: splash_irq.asm | $(OUT)
	$(NASM) -f elf32 $< -o $@

$(OUT)/atapi.o: ../../Drivers/Storage/atapi.c ../../Drivers/Storage/atapi.h ../../Arch/x86/io.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/memory.o: ../../Lib/memory.c | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/js.o: ../../Runtime/JS/js.c ../../Runtime/JS/js.h ../../Runtime/JS/full.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

JS_DIR := ../../Runtime/JS
JS_HEADERS := $(wildcard $(JS_DIR)/compat/*.h) $(wildcard $(JS_DIR)/vendor/quickjs/*.h) $(wildcard $(JS_DIR)/compat/sys/*.h)
$(OUT)/js_full.o: $(JS_DIR)/full.c $(JS_DIR)/full.h $(JS_DIR)/js.h $(JS_HEADERS) | $(OUT)
	$(CLANG) $(CFLAGS) -mno-soft-float -I$(JS_DIR)/compat -Wno-ignored-attributes -Wno-unused-parameter -c $< -o $@

QJS_DIR := $(JS_DIR)/vendor/quickjs
QJS_FLAGS := $(CFLAGS) -std=gnu11 -mno-soft-float -I$(JS_DIR)/compat -Wno-unused-parameter -Wno-sign-compare -Wno-implicit-fallthrough -Wno-ignored-attributes -Wno-macro-redefined -Wno-unused-function -Wno-missing-field-initializers -Wno-unterminated-string-initialization
$(OUT)/quickjs.o: $(JS_DIR)/quickjs.c $(QJS_DIR)/quickjs.c $(wildcard $(QJS_DIR)/*.h) $(JS_HEADERS) | $(OUT)
	$(CLANG) $(QJS_FLAGS) -c $< -o $@
$(OUT)/qjs_cutils.o: $(QJS_DIR)/cutils.c $(JS_HEADERS) | $(OUT)
	$(CLANG) $(QJS_FLAGS) -c $< -o $@
$(OUT)/qjs_dtoa.o: $(QJS_DIR)/dtoa.c $(JS_HEADERS) | $(OUT)
	$(CLANG) $(QJS_FLAGS) -c $< -o $@
$(OUT)/qjs_regexp.o: $(QJS_DIR)/libregexp.c $(JS_HEADERS) | $(OUT)
	$(CLANG) $(QJS_FLAGS) -c $< -o $@
$(OUT)/qjs_unicode.o: $(QJS_DIR)/libunicode.c $(JS_HEADERS) | $(OUT)
	$(CLANG) $(QJS_FLAGS) -c $< -o $@
$(OUT)/js_support.o: $(JS_DIR)/support.c $(JS_HEADERS) | $(OUT)
	$(CLANG) $(CFLAGS) -mno-soft-float -I$(JS_DIR)/compat -Wno-ignored-attributes -Wno-unused-parameter -c $< -o $@

$(OUT)/js_jump.o: $(JS_DIR)/jump.asm | $(OUT)
	$(NASM) -f elf32 $< -o $@

$(OUT)/json.o: ../../Runtime/JS/json.c ../../Runtime/JS/js.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/ttf.o: ../../Runtime/Font/ttf.c ../../Runtime/Font/ttf.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/font.o: ../../Browser/font.c ../../Browser/font.h ../../Runtime/Font/ttf.h ../../Drivers/Video/framebuffer.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

IMAGE_DIR := ../../Runtime/Image
IMAGE_HEADERS := $(wildcard $(IMAGE_DIR)/compat/*.h) $(wildcard $(IMAGE_DIR)/vendor/*.h) $(IMAGE_DIR)/image.h $(IMAGE_DIR)/support.h
$(OUT)/image.o: $(IMAGE_DIR)/image.c $(IMAGE_HEADERS) | $(OUT)
	$(CLANG) $(CFLAGS) -mno-soft-float -I$(IMAGE_DIR)/compat -Wno-unused-function -Wno-sign-compare -c $< -o $@

$(OUT)/image_support.o: $(IMAGE_DIR)/support.c $(IMAGE_HEADERS) | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/images.o: ../../Browser/images.c ../../Browser/images.h ../../Browser/css.h $(IMAGE_DIR)/image.h ../../Drivers/Video/framebuffer.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/font_test.o: ../../Tests/Font/test.c ../../Runtime/Font/ttf.h ../../Browser/font.h ../../Browser/css.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/image_test.o: ../../Tests/Image/test.c ../../Tests/Image/fixtures.h $(IMAGE_DIR)/image.h ../../Browser/images.h ../../Browser/css.h ../../Browser/html.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/js_test.o: ../../Tests/JS/test.c ../../Runtime/JS/js.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/css_test.o: ../../Tests/CSS/test.c ../../Browser/css.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/input_test.o: ../../Tests/Input/test.c ../../Drivers/Input/ps2.h ../../Drivers/Video/framebuffer.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/ps2.o: ../../Drivers/Input/ps2.c ../../Drivers/Input/ps2.h ../../Arch/x86/io.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/framebuffer.o: ../../Drivers/Video/framebuffer.c ../../Drivers/Video/framebuffer.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/css.o: ../../Browser/css.c ../../Browser/css.h ../../Browser/font.h ../../Browser/images.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/html.o: ../../Browser/html.c ../../Browser/html.h ../../Browser/css.h ../../Browser/font.h ../../Runtime/Font/ttf.h ../../Drivers/Video/framebuffer.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/browser.o: ../../Browser/browser.c ../../Browser/browser.h ../../Browser/html.h ../../Browser/css.h ../../Runtime/JS/js.h ../../FS/CDFS/cdfs.h ../../Arch/x86/io.h ../../Drivers/Video/framebuffer.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/atapi_test.o: ../../Tests/ATAPI/test.c ../../FS/CDFS/cdfs.h ../../Drivers/Storage/atapi.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/test.o: ../../Tests/CDFS/test.c ../../FS/CDFS/cdfs.h ../../Drivers/Storage/atapi.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/payload.bin: $(OBJECTS) payload.ld
	$(LLD) -m elf_i386 --gc-sections -T payload.ld --oformat binary $(OBJECTS) -o $@

$(OUT)/stage2.bin: stage2.asm layout.inc $(OUT)/payload.bin | $(OUT)
	$(NASM) -f bin -I ./ -D "PAYLOAD='$(OUT)/payload.bin'" stage2.asm -o $@

previews: | $(OUT)
	$(if $(IMAGE_PREVIEWS),powershell.exe -NoProfile -ExecutionPolicy Bypass -File $(ROOT)/Tools/prepare-images.ps1 -Root "$(ROOT)" -Output "$(IMAGE_PREVIEWS)",@rem Original assets are used in decoder tests.)

$(OUT)/envy.inc: FORCE $(ROOT)/Tools/envy.js previews | $(OUT)
	cscript.exe //nologo $(ROOT)/Tools/envy.js "$(ROOT)" "$@" $(if $(IMAGE_PREVIEWS),"$(IMAGE_PREVIEWS)")

$(OUT)/boot.iso: $(OUT)/stage1.bin $(OUT)/stage2.bin $(ROOT)/Tools/mkimage.bat $(ROOT)/Tools/iso.asm ../../Image/README.TXT layout.inc $(ENVY_TREE)
	cmd.exe /c $(subst /,\,$(ROOT)/Tools/mkimage.bat) "$(OUT)/stage1.bin" "$(OUT)/stage2.bin" "$@" $(if $(ENVY_TREE),"$(ENVY_TREE)")
