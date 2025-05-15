#!/bin/bash
set -e

# Print header
echo "=== SoLoud Test Build Script ==="
echo ""

# Determine the number of jobs to use for faster compilation.
JOBS=1
if command -v nproc >/dev/null; then
    JOBS="$(nproc)"
    echo "Using $JOBS parallel jobs (from nproc)"
elif command -v sysctl >/dev/null && sysctl -n hw.logicalcpu >/dev/null; then
    JOBS="$(sysctl -n hw.logicalcpu)"
    echo "Using $JOBS parallel jobs (from sysctl)"
fi

# Allow being called from any directory.
SCRIPT_DIR="$(dirname "$0")"
cd "$SCRIPT_DIR"
echo "Working directory: $(pwd)"

# Detect OS for platform-specific settings
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Detected macOS system"
    # Check if SDL2 framework is installed
    if [ ! -d "/Library/Frameworks/SDL2.framework" ]; then
        echo "Warning: SDL2 framework not found at /Library/Frameworks/SDL2.framework"
        echo "You may need to install SDL2 with: brew install sdl2"
    fi
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Detected Linux system"
    # Check if SDL2 development package is installed
    if ! pkg-config --exists sdl2; then
        echo "Warning: SDL2 development package not found"
        echo "You may need to install it with: sudo apt-get install libsdl2-dev"
    fi
elif [[ "$OSTYPE" == "msys"* ]] || [[ "$OSTYPE" == "cygwin"* ]] || [[ "$OSTYPE" == "win32"* ]]; then
    echo "Detected Windows system"
    # Windows-specific checks can be added here if needed
fi

# Do the build.
echo "Creating build directory..."
mkdir -p build
cd build

# Configure the build.
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Compile the code.
echo "Compiling with $JOBS parallel jobs..."
if ! cmake --build . -j"$JOBS"; then
    echo "Error: Compilation failed." >&2
    exit 1
fi

echo ""
echo "Build completed successfully!"
echo "You can run the SoLoud test with:"
echo "./build/soloud_test"
echo "" 