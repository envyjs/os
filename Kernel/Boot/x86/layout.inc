; Shared with Tools/iso.asm. Image locations use 2048-byte ISO blocks.
%define ISO_BASE_BLOCKS 512
%define ISO_BLOCKS ISO_BASE_BLOCKS
%define BOOT_LBA 24
%define STAGE2_LBA (BOOT_LBA + 1)
%define STAGE2_SECTORS 384
%define README_LBA (STAGE2_LBA + STAGE2_SECTORS)
%define STAGE2_ADDRESS 0x8000
%define CODE_SELECTOR 0x08
%define DATA_SELECTOR 0x10
