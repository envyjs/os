#include "efi.h"
#include "efilib.h"
#include "buildinfo.h"

typedef void (*kernel_entry_t)(void);

#define EFI_FILE_MODE_READ  0x0000000000000001ULL

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(L"NuKernel Bootloader\n");
    Print(L"Version %s (%s)\n", NUKERNEL_VERSION, NUKERNEL_TIMESTAMP);


    EFI_STATUS status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    EFI_FILE_PROTOCOL *root;
    EFI_FILE_PROTOCOL *kernelFile;

    /* Get filesystem from the device that loaded us */
    status = BS->HandleProtocol(
        ImageHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (void**)&fs
    );
    if (status != 0) {
        Print(L"HandleProtocol failed\n");
        return status;
    }

    status = fs->OpenVolume(fs, &root);
    if (status != 0) {
        Print(L"OpenVolume failed\n");
        return status;
    }

    status = root->Open(root, &kernelFile, L"kernel.bin",
                        EFI_FILE_MODE_READ, 0);
    if (status != 0) {
        Print(L"Failed to open kernel.bin\n");
        return status;
    }

    UINTN kernelSize = 1024 * 1024; /* up to 1 MiB */
    EFI_PHYSICAL_ADDRESS kernelAddr = 0x100000;

    status = BS->AllocatePages(0, 0, kernelSize / 4096, &kernelAddr);
    if (status != 0) {
        Print(L"AllocatePages failed\n");
        return status;
    }

    status = kernelFile->Read(kernelFile, &kernelSize, (void*)kernelAddr);
    if (status != 0) {
        Print(L"Read kernel failed\n");
        return status;
    }

    kernelFile->Close(kernelFile);

    Print(L"Jumping to kernel...\n");

    kernel_entry_t kmain = (kernel_entry_t)(uintptr_t)kernelAddr;
    kmain();

    return 0;
}
