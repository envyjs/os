#!/bin/bash

# Envy Kernel build script for GitHub Actions

# Make all script files in the Tools directory executable (*nix only)
chmod +x ./*.sh
chmod +x ./Compilation/*.sh

# Make a output folder for the compil
mkdir ../Output/
mkdir ../Output/bin/
mkdir ../Output/install/

sudo apt update
sudo apt install -y nasm