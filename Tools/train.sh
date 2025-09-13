#!/bin/bash

# Envy Aiya 2.0 training script for Linux (macOS and Windows not supported)

# Check if Homebrew is installed
if ! command -v pip >/dev/null 2>&1; then
    echo "Python3 not installed. Please install Python3 before running this script."
    exit 1
fi

pip install scikit-learn pandas numpy matplotlib jupyter

python3 ./ML/load.py