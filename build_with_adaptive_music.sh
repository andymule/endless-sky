#!/bin/bash
set -e

echo "=== Building Endless Sky with AdaptiveMusic Integration ==="
echo ""

# Get SoLoud repository if it doesn't exist
if [ ! -d "extern/soloud" ]; then
    echo "Cloning SoLoud repository..."
    mkdir -p extern
    git clone https://github.com/jarikomppa/soloud.git extern/soloud
fi

# Check for SDL2
if [ "$(uname)" == "Darwin" ]; then
    if ! brew list | grep -q sdl2; then
        echo "SDL2 not found! Installing with Homebrew..."
        brew install sdl2
    fi
    
    # Export SDL2 paths for CMake
    export SDL2_DIR="$(brew --prefix sdl2)"
    export CMAKE_PREFIX_PATH="$SDL2_DIR:$CMAKE_PREFIX_PATH"
    export CXXFLAGS="-I$SDL2_DIR/include/SDL2 $CXXFLAGS"
    export CFLAGS="-I$SDL2_DIR/include/SDL2 $CFLAGS"
fi

# Determine number of build jobs
JOBS=1
if command -v nproc >/dev/null; then
    JOBS="$(nproc)"
elif command -v sysctl >/dev/null && sysctl -n hw.logicalcpu >/dev/null; then
    JOBS="$(sysctl -n hw.logicalcpu)"
fi
echo "Using $JOBS parallel jobs for building"

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building Endless Sky..."
cmake --build . -j"$JOBS"

echo ""
echo "Build completed! You can run Endless Sky with AdaptiveMusic by executing:"
if [ "$(uname)" == "Darwin" ]; then
    echo "./build/Endless\\ Sky.app/Contents/MacOS/Endless\\ Sky"
else
    echo "./build/endless-sky"
fi
echo "" 