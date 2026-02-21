@echo off
setlocal enabledelayedexpansion

echo ============================================================
echo   NuKernel Build System
echo   Starting build...
echo ============================================================

rem --- Paths ---
set SRC_BOOT=Kernel\boot
set SRC_KRNL=Kernel\krnl
set SRC=Kernel\
set OUT=Output
set ISO=Output\ISO

for /f "delims=" %%b in ('git rev-parse --abbrev-ref HEAD') do set BRANCH=%%b

rem --- Ensure build and ISO directories exist ---
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%ISO%\EFI\BOOT" mkdir "%ISO%\EFI\BOOT"

set VERSION_FILE=%SRC%\version.txt

set /p NUKERNEL_VERSION=<"%VERSION_FILE%"

echo Version: %NUKERNEL_VERSION%

echo.
echo === Generating timestamp ===

rem === Generate NT-style timestamp: YYMMDD-HHMM ===
for /f "tokens=1-4 delims=/ " %%a in ("%date%") do (
    set DD=%%a
    set MM=%%b
    set YYYY=%%c
)

for /f "tokens=1-2 delims=:." %%a in ("%time%") do (
    set HH=%%a
    set MIN=%%b
)

if "%HH:~0,1%"==" " set HH=0%HH:~1,1%

set YY=%YYYY:~2,2%
set TIMESTAMP=%YY%%MM%%DD%-%HH%%MIN%

echo Timestamp: %TIMESTAMP%

echo.
echo === Generating buildinfo.h ===

echo #pragma once > "%SRC_BOOT%\buildinfo.h"
echo #define NUKERNEL_VERSION L"%NUKERNEL_VERSION%" >> "%SRC_BOOT%\buildinfo.h"
echo #define NUKERNEL_TIMESTAMP L"%TIMESTAMP%" >> "%SRC_BOOT%\buildinfo.h"

echo buildinfo.h generated.

echo.
echo === Building UEFI bootloader ===

clang -target x86_64-pc-win32-coff -fshort-wchar -mno-red-zone -ffreestanding ^
  -I "%SRC_BOOT%" -I "%SRC_KRNL%" -c "%SRC_BOOT%\main.c" -o "%OUT%\main.obj"

clang -target x86_64-pc-win32-coff -fshort-wchar -mno-red-zone -ffreestanding ^
  -I "%SRC_BOOT%" -I "%SRC_KRNL%" -c "%SRC_BOOT%\efilib.c" -o "%OUT%\efilib.obj"

lld-link /subsystem:efi_application /entry:efi_main /machine:x64 ^
  /out:"%OUT%\BOOTX64.EFI" ^
  "%OUT%\main.obj" "%OUT%\efilib.obj"

copy "%OUT%\BOOTX64.EFI" "%ISO%\EFI\BOOT\BOOTX64.EFI" >nul

echo.
echo ISO/EFI/BOOT contents:
dir "%ISO%\EFI\BOOT"

echo.
echo === Building kernel ===

clang -target x86_64-elf -ffreestanding -fno-stack-protector -mno-red-zone ^
  -I "%SRC_KRNL%" -c "%SRC_KRNL%\kernel.c" -o "%OUT%\kernel.o"

ld.lld -nostdlib -z max-page-size=0x1000 ^
  -T "%SRC_KRNL%\kernel.ld" ^
  "%OUT%\kernel.o" -o "%OUT%\kernel.elf"

llvm-objcopy -O binary "%OUT%\kernel.elf" "%OUT%\kernel.bin"

copy "%OUT%\kernel.bin" "%ISO%\kernel.bin" >nul

echo.
echo ISO root contents:
dir "%ISO%"

echo.
echo === Creating ISO ===

set ISOFILE=%NUKERNEL_VERSION%-%BRANCH%-%TIMESTAMP%.iso

echo Creating ISO: %ISOFILE%

tools\oscdimg -n -m -o ^
  -bootdata:1#pEF,e,b"%ISO%\EFI\BOOT\BOOTX64.EFI" ^
  "%ISO%" "%OUT%\%ISOFILE%"

echo.
echo ============================================================
echo   Build complete
echo   Version:     %NUKERNEL_VERSION%
echo   Timestamp:   %TIMESTAMP%
echo   ISO image:   %OUT%\%ISOFILE%
echo ============================================================

endlocal