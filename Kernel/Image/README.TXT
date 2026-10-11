Welcome to the from-scratch kernel bootloader.
This text was read from /DOCS/README.TXT on the CDFS boot image.

The x86 boot image supports El Torito optical boot and legacy BIOS USB-HDD
boot. To boot from a USB drive, raw-write Build/Boot/x86/boot.iso to the drive;
copying the ISO file onto a formatted drive does not install its boot sector.
The destination drive is overwritten. UEFI-only systems are not supported.
