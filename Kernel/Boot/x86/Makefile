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
CFLAGS := --target=i686-none-elf -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -mno-sse -mno-mmx -msoft-float -std=c11 -Wall -Wextra -Werror -Oz -ffunction-sections -fdata-sections
OBJECTS := $(OUT)/payload.o $(OUT)/boot.o $(OUT)/cdfs.o $(OUT)/atapi.o $(OUT)/memory.o $(OUT)/js.o $(OUT)/browser.o $(OUT)/framebuffer.o $(OUT)/html.o $(OUT)/css.o $(OUT)/ps2.o $(OUT)/ttf.o $(OUT)/font.o $(OUT)/image.o $(OUT)/image_support.o $(OUT)/images.o
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
all: $(OUT)/boot.iso

$(OUT):
	cmd.exe /c if not exist "$(subst /,\,$(OUT))" mkdir "$(subst /,\,$(OUT))"

$(OUT)/stage1.bin: stage1.asm layout.inc | $(OUT)
	$(NASM) -f bin -I ./ stage1.asm -o $@

$(OUT)/cdfs.o: ../../FS/CDFS/cdfs.c ../../FS/CDFS/cdfs.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/boot.o: boot.c ../../FS/CDFS/cdfs.h ../../Drivers/Storage/atapi.h ../../Arch/x86/io.h ../../Browser/browser.h ../../Drivers/Video/framebuffer.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/payload.o: payload.asm | $(OUT)
	$(NASM) -f elf32 $< -o $@

$(OUT)/atapi.o: ../../Drivers/Storage/atapi.c ../../Drivers/Storage/atapi.h ../../Arch/x86/io.h | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/memory.o: ../../Lib/memory.c | $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/js.o: ../../Runtime/JS/js.c ../../Runtime/JS/js.h | $(OUT)
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
