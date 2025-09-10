#!/bin/bash
# Envy Kernel compilation script for i386 (x86-32)
echo "Building Envy Kernel for i386 (x86-32)"
# Build the neutral components first (Drivers, SysNeutral, etc)
mkdir ../Output/bin/i386/system/
exec nasm ../Kernel/System/i386/BIOS/*.asm -o ../Output/bin/i386/system/legboot.app