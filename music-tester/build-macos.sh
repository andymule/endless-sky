#!/bin/bash
set -e

# Determine architecture
ARCH=$(uname -m)
echo "Detected architecture: $ARCH"

# Set minizip triplet based on architecture
if [ "$ARCH" == "arm64" ]; then
    MINIZIP_TRIPLET="arm64-osx"
else
    MINIZIP_TRIPLET="x64-osx"
fi
echo "Using minizip triplet: $MINIZIP_TRIPLET"

# Ensure we're in the project root
cd "$(dirname "$0")/.."
ROOT_DIR=$(pwd)
echo "Project root: $ROOT_DIR"

# Clean build directory if clean flag is provided
if [ "$1" == "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

# Create build directory if it doesn't exist
mkdir -p build

# Initialize vcpkg if needed
if [ ! -f "$ROOT_DIR/vcpkg/vcpkg" ]; then
    echo "Initializing vcpkg..."
    # If vcpkg is a submodule, update it
    if [ -f "$ROOT_DIR/.gitmodules" ]; then
        git submodule update --init --recursive
    else
        # Clone vcpkg if not a submodule
        git clone https://github.com/microsoft/vcpkg.git
        "$ROOT_DIR/vcpkg/bootstrap-vcpkg.sh"
    fi
fi

# Make sure the required system packages are installed
echo "Checking for required system libraries..."
MISSING_LIBS=()

# Check for libogg
if ! brew list libogg >/dev/null 2>&1; then
    MISSING_LIBS+=("libogg")
fi

# Check for libvorbis and vorbis-tools
if ! brew list libvorbis vorbis-tools >/dev/null 2>&1; then
    MISSING_LIBS+=("libvorbis vorbis-tools")
fi

# Check for pkg-config
if ! brew list pkg-config >/dev/null 2>&1; then
    MISSING_LIBS+=("pkg-config")
fi

# Install missing libraries if needed
if [ ${#MISSING_LIBS[@]} -gt 0 ]; then
    echo "Installing required system libraries via Homebrew: ${MISSING_LIBS[*]}"
    brew install ${MISSING_LIBS[*]}
fi
    
# Check if OAML is installed, if not print a message
if ! brew list oaml >/dev/null 2>&1; then
    echo "OAML is required but not installed via Homebrew."
    echo "Please ensure it's properly installed at /usr/local/lib/liboaml.dylib"
    
    # Check if manually installed
    if [ -f "/usr/local/lib/liboaml.dylib" ]; then
        echo "Found manually installed OAML library."
    else
        echo "WARNING: Could not find OAML library. Build may fail."
        echo "You can install it manually with:"
        echo ""
        echo "git clone https://github.com/oamldev/oaml.git"
        echo "cd oaml"
        echo "mkdir build && cd build"
        echo "cmake .. -DENABLE_SHARED=ON -DENABLE_STATIC=ON"
        echo "make"
        echo "sudo make install"
    fi
fi

# Make sure vorbis libraries are found
echo "Checking vorbis and ogg library configurations..."
pkg-config --cflags --libs vorbis vorbisfile ogg

# Force install just minizip (override manifest mode)
echo "Installing minizip via vcpkg in classic mode..."
"$ROOT_DIR/vcpkg/vcpkg" install minizip --triplet=$MINIZIP_TRIPLET --classic --no-print-usage

# Make sure the minizip path exists
MINIZIP_DIR="$ROOT_DIR/vcpkg/installed/${MINIZIP_TRIPLET}/share/unofficial-minizip"
if [ ! -d "$MINIZIP_DIR" ]; then
    echo "ERROR: Minizip directory not found at $MINIZIP_DIR"
    echo "Trying alternative location..."
    MINIZIP_DIR="$ROOT_DIR/vcpkg/packages/minizip_${MINIZIP_TRIPLET}/share/unofficial-minizip"
    if [ ! -d "$MINIZIP_DIR" ]; then
        echo "ERROR: Minizip directory not found at alternative location. Exiting."
        exit 1
    fi
fi

# For manifest mode features (needed for other vcpkg dependencies)
export VCPKG_FEATURE_FLAGS=versions
export VCPKG_MANIFEST_FEATURES="system-libs"

# Clean CMake cache to force reconfiguration
if [ "$1" == "clean" ]; then
    rm -f build/CMakeCache.txt
fi

# Set PKG_CONFIG_PATH to include Homebrew library paths
BREW_PREFIX=$(brew --prefix)
export PKG_CONFIG_PATH="${BREW_PREFIX}/lib/pkgconfig:${BREW_PREFIX}/opt/libvorbis/lib/pkgconfig:${BREW_PREFIX}/opt/libogg/lib/pkgconfig:${PKG_CONFIG_PATH}"
echo "Using PKG_CONFIG_PATH: ${PKG_CONFIG_PATH}"

echo "Configuring project..."
# Configure CMake with explicit minizip path
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_POLICY_DEFAULT_CMP0077=NEW \
  -DES_BUILD_MUSIC_TESTER=ON \
  -DES_USE_VCPKG=ON \
  -Dunofficial-minizip_DIR="$MINIZIP_DIR"

echo "Building music-tester..."
cmake --build build --target music-tester -j$(sysctl -n hw.ncpu)

echo "Build complete. You can run the music-tester with:"
echo "./build/music-tester"
echo ""
echo "To clean and rebuild, run this script with the 'clean' argument:"
echo "./music-tester/build-macos.sh clean" 