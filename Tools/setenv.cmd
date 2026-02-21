@echo off
rem ============================================================
rem   NuKernel Build Environment Initialisation (setenv.cmd)
rem ============================================================

setlocal

rem --- Resolve project root (parent of tools\) ---
pushd "%~dp0.."
set PROJECTROOT=%CD%
popd

rem --- Toolchain paths ---
set TOOLSDIR=%PROJECTROOT%\tools
set PATH=%TOOLSDIR%;%PATH%

rem --- Source tree ---
set SRC_BOOT=%PROJECTROOT%\Kernel\boot
set SRC_KRNL=%PROJECTROOT%\Kernel\krnl

rem --- Output directories ---
set OUT=%PROJECTROOT%\Output
set ISO=%OUT%\ISO

rem --- Build configuration ---
set BUILDMODE=DEBUG

echo.
echo ============================================================
echo   NuKernel Build Environment Loaded
echo   Project Root: %PROJECTROOT%
echo   Build Mode:   %BUILDMODE%
echo ============================================================
echo.

endlocal & (
    set PROJECTROOT=%PROJECTROOT%
    set TOOLSDIR=%TOOLSDIR%
    set SRC_BOOT=%SRC_BOOT%
    set SRC_KRNL=%SRC_KRNL%
    set OUT=%OUT%
    set ISO=%ISO%
    set BUILDMODE=%BUILDMODE%
    set PATH=%PATH%
)