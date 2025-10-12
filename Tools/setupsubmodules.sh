#!/bin/bash

# Envy submodule setup script for macOS/Linux
# This script initializes and updates the git submodules required for the Envy project.
if [[ "$OSTYPE" == "darwin"* ]]; then 
    echo "macOS detected, setting up submodules..."
    exit 1
elif [[ "$OSTYPE" == "linux"* ]]; then 
    echo "Linux detected, setting up submodules..."
    exit 1
elif [[ "$OSTYPE" == "msys"* ]]; then 
    echo "Windows is not supported, quitting"
    exit 1
else
    echo "Unknown operating system, quitting"
fi