#!/bin/bash
# Envy Kernel compilation script for AMD64
echo "Building Envy Kernel for AMD64"
# Build the neutral components first (Drivers, SysNeutral, etc
mkdir ../Output/bin/AMD64/system/
exec nasm ../Kernel/System/AMD64/BIOS/*.asm -o ../Output/bin/AMD64/system/legboot.app