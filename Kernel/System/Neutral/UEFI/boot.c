// File: boot.c
// Purpose: Initialises boot for UEFI systems (all architectures)
// Copyright Envy Group 2022-2025, licensed under the Envy Public License
#include <efi.h>
#include <efilib.h>
#include <./Neutral/ver.h>

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    // Initialize the GNU-EFI library
    InitializeLib(ImageHandle, SystemTable);
    Print(L"Envy Kernel (Version %d.%d.%d.%d)\n", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD, VERSION_REV);

    // Wait for a keypress to prevent the app from exiting immediately
    SystemTable->ConIn->Reset(SystemTable->ConIn, FALSE);
    while (SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, NULL) != EFI_SUCCESS);

    return EFI_SUCCESS;
}