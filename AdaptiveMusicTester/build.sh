#!/bin/bash

# Stop on errors
set -e

# Check if ImGui is present, if not clone it
if [ ! -d "imgui" ]; then
    echo "ImGui not found, cloning..."
    git clone https://github.com/ocornut/imgui.git
fi

# Check if SoLoud is present, if not clone it
if [ ! -d "soloud" ]; then
    echo "SoLoud not found, cloning..."
    git clone https://github.com/jarikomppa/soloud.git
fi

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Try to detect SDL path type
SDL_INCLUDE_DIR="$(sdl2-config --cflags 2>/dev/null | grep -o '/[^ ]*' | head -n 1)" || true

if [ -n "$SDL_INCLUDE_DIR" ] && [ -d "$SDL_INCLUDE_DIR/SDL2" ]; then
    echo "Detected SDL2 subdirectory include style"
    CMAKE_ARGS="-DUSE_SDL2_SUBDIR=ON"
else
    echo "Detected direct SDL include style"
    CMAKE_ARGS="-DUSE_SDL2_SUBDIR=OFF"
fi

# Configure and build
cmake .. $CMAKE_ARGS
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)

echo ""
echo "Build complete!"
echo ""
echo "To run the application, use:"
echo "cd build && ./adaptive_music_tester" 