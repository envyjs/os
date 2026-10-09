SHELL := cmd.exe
.SHELLFLAGS := /c
.DEFAULT_GOAL := all

NASM ?= nasm
# Support NASM's standard Windows installer location without a PATH edit.
ifeq ($(NASM),nasm)
ifneq ($(wildcard C:/Program\ Files/NASM/nasm.exe),)
NASM := "C:/Program Files/NASM/nasm.exe"
endif
endif
CLANG ?= clang
LLD ?= ld.lld
QEMU ?= qemu-system-i386
QEMU_DISPLAY ?= sdl
QEMU_FLAGS ?=
export NASM CLANG LLD

.PHONY: all kernel run test font-test image-test
all: kernel

kernel:
	$(MAKE) -C Kernel all

run: all
	"$(QEMU)" -name "Envy NuKernel x86-32" -m 256 -display $(QEMU_DISPLAY) -drive file=Build/Boot/x86/boot.iso,format=raw,media=cdrom,readonly=on -boot d $(QEMU_FLAGS)

test:
	$(MAKE) -C Kernel/Boot/x86 OUT=../../../Build/Tests/CDFS TESTS=1 all
	cmd.exe /c Tools\test-cdfs.bat "$(QEMU)" "Build/Tests/CDFS/boot.iso"

font-test:
	$(MAKE) -C Kernel/Boot/x86 OUT=../../../Build/Tests/Font FONT_TESTS=1 all
	cmd.exe /c Tools\test-cdfs.bat "$(QEMU)" "Build/Tests/Font/boot.iso"

image-test:
	$(MAKE) -C Kernel/Boot/x86 OUT=../../../Build/Tests/Image IMAGE_TESTS=1 all
	cmd.exe /c Tools\test-cdfs.bat "$(QEMU)" "Build/Tests/Image/boot.iso"
