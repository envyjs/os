; Shared with Tools/iso.asm. CD LBAs use 2048-byte blocks.
%define ISO_BLOCKS 128
%define BOOT_LBA 24
%define STAGE2_LBA (BOOT_LBA + 1)
%define STAGE2_SECTORS 96
%define README_LBA (STAGE2_LBA + STAGE2_SECTORS)
%define STAGE2_ADDRESS 0x8000
%define CODE_SELECTOR 0x08
%define DATA_SELECTOR 0x10
