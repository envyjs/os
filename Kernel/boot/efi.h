#ifndef _EFI_H
#define _EFI_H

#include <stdint.h>

#define EFIAPI __attribute__((ms_abi))

typedef uint64_t EFI_STATUS;
typedef void* EFI_HANDLE;
typedef uint16_t CHAR16;

typedef uint64_t EFI_PHYSICAL_ADDRESS;
typedef uint64_t UINTN;
typedef uint64_t UINT64;
typedef uint32_t UINT32;
typedef uint8_t  UINT8;

/* Text output protocol */
struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_STATUS (EFIAPI *Reset)(
        struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
        uint8_t ExtendedVerification
    );

    EFI_STATUS (EFIAPI *OutputString)(
        struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
        CHAR16 *String
    );
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

/* GUID */
typedef struct {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
} EFI_GUID;

/* Forward decls */
struct _EFI_FILE_PROTOCOL;
struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
struct _EFI_BOOT_SERVICES;

/* File + FS protocols */
typedef struct _EFI_FILE_PROTOCOL {
    EFI_STATUS (EFIAPI *Open)(
        struct _EFI_FILE_PROTOCOL *This,
        struct _EFI_FILE_PROTOCOL **NewHandle,
        CHAR16 *FileName,
        UINT64 OpenMode,
        UINT64 Attributes
    );
    EFI_STATUS (EFIAPI *Close)(struct _EFI_FILE_PROTOCOL *This);
    EFI_STATUS (EFIAPI *Read)(
        struct _EFI_FILE_PROTOCOL *This,
        UINTN *BufferSize,
        void *Buffer
    );
} EFI_FILE_PROTOCOL;

typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    EFI_STATUS (EFIAPI *OpenVolume)(
        struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
        EFI_FILE_PROTOCOL **Root
    );
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

/* Boot services */
typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(
    EFI_HANDLE Handle,
    EFI_GUID *Protocol,
    void **Interface
);

typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_PAGES)(
    int Type,
    int MemoryType,
    UINTN Pages,
    EFI_PHYSICAL_ADDRESS *Memory
);

typedef struct _EFI_BOOT_SERVICES {
    char _pad[24]; /* skip stuff we don't use */
    EFI_ALLOCATE_PAGES AllocatePages;
    char _pad2[56]; /* skip to HandleProtocol */
    EFI_HANDLE_PROTOCOL HandleProtocol;
} EFI_BOOT_SERVICES;

/* System table (single, final definition) */
typedef struct _EFI_SYSTEM_TABLE {
    char _buf1[44];  /* Skip headers we don't need */
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    void *Reserved;              /* keep alignment sane */
    EFI_BOOT_SERVICES *BootServices; /* <- added */
} EFI_SYSTEM_TABLE;

/* Globals we’ll use */
extern EFI_SYSTEM_TABLE *ST;
extern EFI_BOOT_SERVICES *BS;
extern EFI_GUID gEfiSimpleFileSystemProtocolGuid;

#endif
