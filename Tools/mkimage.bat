@echo off
setlocal EnableExtensions
if "%~3"=="" goto usage
if not "%~5"=="" goto usage
if not exist "%~1" (
    echo ERROR: First-stage file does not exist.
    exit /b 1
)
if not exist "%~2" (
    echo ERROR: Second-stage file does not exist.
    exit /b 1
)
if not "%~z1"=="2048" (
    echo ERROR: First stage must be exactly 2048 bytes.
    exit /b 1
)
rem The second-stage size is validated by iso.asm against shared layout.inc.
rem NASM comes from the invoking makefile, or from PATH for direct use.
if not defined NASM set "NASM=nasm"
set "ENVY_ARG="
if not "%~4"=="" set "ENVY_ARG=-D ENVY_TREE='%~f4'"
%NASM% -f bin -I "%~dp0..\Kernel\Boot\x86/" -D "STAGE1='%~f1'" -D "STAGE2='%~f2'" -D "README='%~dp0..\Kernel\Image\README.TXT'" %ENVY_ARG% "%~dp0iso.asm" -o "%~f3"
if errorlevel 1 exit /b 1
if "%~4"=="" if not "%~z3"=="262144" (
    echo ERROR: Incorrect output image size.
    exit /b 1
)
echo Created %~3 ^(ISO 9660 / El Torito, %~z3 bytes^)
exit /b 0

:usage
echo Usage: mkimage.bat stage1.bin stage2.bin boot.iso [envy.inc]
exit /b 1
