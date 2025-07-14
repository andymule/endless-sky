#!/bin/bash

# Cross-platform build script that works on macOS, Linux, and Windows (with MSYS2)

set -e

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    echo "Building on macOS..."
    ./build.sh
    ./build/dynamix
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    echo "Building on Linux..."
    ./build.sh
    ./build/dynamix
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    # Windows with MSYS2
    echo "Building on Windows with MSYS2..."
    ./build.sh
    ./build/dynamix.exe
else
    echo "Unsupported OS: $OSTYPE"
    exit 1
fi 