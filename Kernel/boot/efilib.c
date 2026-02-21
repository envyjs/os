#include "efi.h"
#include "efilib.h"

EFI_SYSTEM_TABLE *ST;
EFI_BOOT_SERVICES *BS;

EFI_GUID gEfiSimpleFileSystemProtocolGuid =
    (EFI_GUID){ 0x0964e5b2, 0x6459, 0x11d2, {0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b} };

EFI_STATUS EFIAPI InitializeLib(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    ST = SystemTable;
    BS = SystemTable->BootServices;
    return 0;
}

EFI_STATUS EFIAPI Print(CHAR16 *fmt, ...) {
    ST->ConOut->OutputString(ST->ConOut, fmt);
    return 0;
}
