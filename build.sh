#!/bin/bash
set -e  # Exit on error

echo "=== Building Endless Sky with AdaptiveMusicLib ==="
echo ""

# Check for prerequisites
if [ ! -d "extern/soloud" ]; then
    echo "Cloning SoLoud repository..."
    mkdir -p extern
    git clone https://github.com/jarikomppa/soloud.git extern/soloud
fi

if [ ! -d "vcpkg" ]; then
    echo "Initializing vcpkg..."
    git clone https://github.com/Microsoft/vcpkg.git
    ./vcpkg/bootstrap-vcpkg.sh -disableMetrics
fi

# Check if minizip is installed
if [ ! -d "vcpkg/packages/minizip_arm64-osx" ]; then
    echo "Installing minizip dependency..."
    ./vcpkg/vcpkg install minizip:arm64-osx --classic
fi

# Check for SDL2 on macOS
if [ "$(uname)" == "Darwin" ]; then
    if ! brew list | grep -q sdl2; then
        echo "Installing SDL2 with Homebrew..."
        brew install sdl2
    fi
    export SDL2_DIR="$(brew --prefix sdl2)"
    export CMAKE_PREFIX_PATH="$SDL2_DIR:$CMAKE_PREFIX_PATH"
    echo "Using SDL2 from: $SDL2_DIR"
fi

# Build AdaptiveMusicLib only if it doesn't exist
if [ ! -f "standalone_adaptive_music_lib/build/libadaptivemusiclib.a" ]; then
    echo "Building standalone AdaptiveMusicLib..."
    mkdir -p standalone_adaptive_music_lib/build
    cd standalone_adaptive_music_lib
    cmake -B build
    cmake --build build -j$(sysctl -n hw.ncpu)
    cd ..
else
    echo "AdaptiveMusicLib already built, skipping..."
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir -p build
fi

# Configure with CMake if not already configured
if [ ! -f "build/CMakeCache.txt" ]; then
    echo "Configuring Endless Sky with CMake..."
    cmake -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
        -Dunofficial-minizip_DIR=$(pwd)/vcpkg/packages/minizip_arm64-osx/share/unofficial-minizip \
        -DBUILD_TESTING=OFF
fi

# Build only the main executable
echo "Building Endless Sky..."
cmake --build build -j$(sysctl -n hw.ncpu) --target EndlessSky

echo ""
echo "Build complete! Run with ./build/endless-sky"
echo "The game will play main_menu.mp3 when launched." 