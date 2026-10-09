@echo off
setlocal EnableExtensions
if "%~2"=="" exit /b 1
set "QEMU=%~1"
set "ISO=%~2"
set "LOG_DIR=%~dp2"
for %%I in (0 1 2 3) do (
    call :position %%I
    if errorlevel 1 exit /b 1
)
echo Testing an ATA disk and an empty CD alongside the boot CD...
"%QEMU%" -m 256 -display none -monitor none -serial none -debugcon "file:%LOG_DIR%atapi-mixed.log" -no-reboot -device isa-debug-exit,iobase=0xf4,iosize=4 -drive "file=%ISO%,format=raw,if=ide,index=0,media=disk,snapshot=on" -drive if=ide,index=1,media=cdrom -drive "file=%ISO%,format=raw,if=ide,index=3,media=cdrom,readonly=on" -boot d
call :result
if errorlevel 1 exit /b 1
echo Native guest tests passed.
exit /b 0

:position
echo Testing native CD reads at IDE index %~1...
"%QEMU%" -m 256 -display none -monitor none -serial none -debugcon "file:%LOG_DIR%atapi-index-%~1.log" -no-reboot -device isa-debug-exit,iobase=0xf4,iosize=4 -drive "file=%ISO%,format=raw,if=ide,index=%~1,media=cdrom,readonly=on" -boot d
goto result

:result
if errorlevel 34 goto failed
if not errorlevel 33 goto failed
exit /b 0
:failed
echo ERROR: Native guest tests failed. QEMU exit code: %errorlevel%
exit /b 1
