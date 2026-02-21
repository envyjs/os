#ifndef _EFILIB_H
#define _EFILIB_H

#include "efi.h"

EFI_STATUS EFIAPI InitializeLib(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);
EFI_STATUS EFIAPI Print(CHAR16 *fmt, ...);

#endif
