#!/bin/bash

# Envy Kernel build script for macOS/Linux
# Make all script files in the Tools directory executable (*nix only)
chmod +x ./*.sh
chmod +x ./Compilation/*.sh

# Make a output folder for the compil
mkdir ../Output/
mkdir ../Output/bin/
mkdir ../Output/boot/

# Check the operating system
if [[ "$OSTYPE" == "darwin"* ]]; then
# Check if Homebrew is installed
if ! command -v brew >/dev/null 2>&1; then
    echo "Homebrew not found. Please install Homebrew first."
    exit 1
fi
# If Homebrew is found, install nasm
echo "Homebrew found. Installing nasm..."
brew install nasm
exec ./arch.sh
elif [[ "$OSTYPE" == "linux"* ]]; then
    if [[ -f /etc/os-release ]]; then
    . /etc/os-release
    if [[ "$ID" == "fedora" || "$ID_LIKE" == *"fedora"* ]]; then
        # Check if dnf is available
        if ! command -v dnf >/dev/null 2>&1; then
            echo "dnf not found. Please ensure dnf is installed."
            exit 1
        fi
        echo "Fedora-based distribution detected. Installing nasm with dnf..."
        sudo dnf install -y nasm
        exec ./arch.sh
    # Check for Debian-based distro
    elif [[ "$ID" == "debian" || "$ID_LIKE" == *"debian"* ]]; then
        # Check if apt is available
        if ! command -v apt >/dev/null 2>&1; then
            echo "apt not found. Please ensure apt is installed."
            exit 1
        fi
        echo "Debian-based distribution detected. Installing nasm with apt..."
        sudo apt update
        sudo apt install -y nasm
        exec ./arch.sh
    # Check for Arch-based distro
    elif [[ "$ID" == "arch" || "$ID_LIKE" == *"arch"* ]]; then
        # Check if pacman is available
        if ! command -v pacman >/dev/null 2>&1; then
            echo "pacman not found. Please ensure pacman is installed."
            exit 1
        fi
        echo "Arch-based distribution detected. Installing nasm with pacman..."
        sudo pacman -S nasm
        exec ./arch.sh
    else
        echo "Unsupported Linux distribution. This script supports Fedora-based and Debian-based distros only."
        exit 1
    fi  
else
    echo "Cannot determine Linux distribution. /etc/os-release not found."
    exit 1
fi
elif [[ "$OSTYPE" == "msys"* ]]; then 
    echo "Windows is not supported, quitting."
    exit 1
fi
else
    echo "Unknown operating system, quitting"
fi