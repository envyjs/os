@echo off
call "%~dp0setenv.cmd"

echo.
echo ================================================
echo   NuKernel Developer Shell Activated
echo   Root: %PROJECTROOT%
echo   Mode: %BUILDMODE%
echo ================================================
echo.

title NuKernel Build Environment

cmd /k