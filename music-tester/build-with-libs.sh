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

# Get Homebrew prefix
BREW_PREFIX=$(brew --prefix)
echo "Homebrew prefix: $BREW_PREFIX"

# Set explicit library paths
export LIBRARY_PATH="${BREW_PREFIX}/lib:/usr/local/lib:${LIBRARY_PATH}"
export LD_LIBRARY_PATH="${BREW_PREFIX}/lib:/usr/local/lib:${LD_LIBRARY_PATH}"
export DYLD_LIBRARY_PATH="${BREW_PREFIX}/lib:/usr/local/lib:${DYLD_LIBRARY_PATH}"
export DYLD_FALLBACK_LIBRARY_PATH="${BREW_PREFIX}/lib:/usr/local/lib:${DYLD_FALLBACK_LIBRARY_PATH}"
export PKG_CONFIG_PATH="${BREW_PREFIX}/lib/pkgconfig:${BREW_PREFIX}/opt/libvorbis/lib/pkgconfig:${BREW_PREFIX}/opt/libogg/lib/pkgconfig:/usr/local/lib/pkgconfig:${PKG_CONFIG_PATH}"

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

# Check for required libs
echo "Checking for required libraries..."
for lib in libogg libvorbis vorbis-tools pkg-config; do
    if ! brew list $lib &>/dev/null; then
        echo "Installing $lib..."
        brew install $lib
    else
        echo "$lib is installed"
    fi
done

# Verify OAML is installed
if [ -f "/usr/local/lib/liboaml.dylib" ]; then
    echo "Found OAML library at /usr/local/lib/liboaml.dylib"
    OAML_LIBRARY="/usr/local/lib/liboaml.dylib"
    OAML_INCLUDE_DIR="/usr/local/include"
else
    echo "WARNING: OAML library not found at /usr/local/lib/liboaml.dylib"
    echo "Please install it manually."
    exit 1
fi

# Verify vorbis libs
ls -la "${BREW_PREFIX}/lib/libvorbis"* || echo "Vorbis libraries not found in ${BREW_PREFIX}/lib"
ls -la "${BREW_PREFIX}/lib/libogg"* || echo "Ogg libraries not found in ${BREW_PREFIX}/lib"

# Display library paths
echo "LIBRARY_PATH: $LIBRARY_PATH"
echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
echo "DYLD_LIBRARY_PATH: $DYLD_LIBRARY_PATH"
echo "PKG_CONFIG_PATH: $PKG_CONFIG_PATH"

# Force install minizip via vcpkg in classic mode
echo "Installing minizip via vcpkg in classic mode..."
"$ROOT_DIR/vcpkg/vcpkg" install minizip --triplet=$MINIZIP_TRIPLET --classic --no-print-usage

# Make sure the minizip path exists
MINIZIP_DIR="$ROOT_DIR/vcpkg/installed/${MINIZIP_TRIPLET}/share/unofficial-minizip"
if [ ! -d "$MINIZIP_DIR" ]; then
    echo "Trying alternative location for minizip..."
    MINIZIP_DIR="$ROOT_DIR/vcpkg/packages/minizip_${MINIZIP_TRIPLET}/share/unofficial-minizip"
    if [ ! -d "$MINIZIP_DIR" ]; then
        echo "ERROR: Minizip directory not found. Searching for it..."
        find "$ROOT_DIR/vcpkg" -name "unofficial-minizip" -type d
        exit 1
    fi
fi
echo "Found minizip at: $MINIZIP_DIR"

# Create simple CMake file that uses explicit paths
cat > build/FindVorbis.cmake << EOF
# FindVorbis.cmake - Explicit paths for vorbis libraries
set(OGG_LIBRARY "${BREW_PREFIX}/lib/libogg.dylib" CACHE FILEPATH "Ogg library")
set(VORBIS_LIBRARY "${BREW_PREFIX}/lib/libvorbis.dylib" CACHE FILEPATH "Vorbis library")
set(VORBISFILE_LIBRARY "${BREW_PREFIX}/lib/libvorbisfile.dylib" CACHE FILEPATH "VorbisFile library")
set(VORBIS_INCLUDE_DIR "${BREW_PREFIX}/include" CACHE PATH "Vorbis include directory")
set(OGG_INCLUDE_DIR "${BREW_PREFIX}/include" CACHE PATH "Ogg include directory")
EOF

# Create a custom FindOAML.cmake file
cat > build/FindOAML.cmake << EOF
# FindOAML.cmake - Explicit paths for OAML
set(OAML_LIBRARY "/usr/local/lib/liboaml.dylib" CACHE FILEPATH "OAML library")
set(OAML_INCLUDE_DIR "/usr/local/include" CACHE PATH "OAML include directory")
set(OAML_FOUND TRUE)
EOF

# Link the vorbis libraries into /usr/local/lib if needed
if [ ! -f "/usr/local/lib/libvorbis.dylib" ]; then
    echo "Creating symbolic links to vorbis libraries in /usr/local/lib"
    sudo ln -sf "${BREW_PREFIX}/lib/libvorbis.dylib" /usr/local/lib/
    sudo ln -sf "${BREW_PREFIX}/lib/libvorbisfile.dylib" /usr/local/lib/
    sudo ln -sf "${BREW_PREFIX}/lib/libogg.dylib" /usr/local/lib/
fi

echo "Configuring project..."
# Configure CMake with explicit paths
cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_MODULE_PATH="${ROOT_DIR}/build" \
  -DCMAKE_POLICY_DEFAULT_CMP0077=NEW \
  -DES_BUILD_MUSIC_TESTER=ON \
  -DES_USE_VCPKG=ON \
  -Dunofficial-minizip_DIR="$MINIZIP_DIR" \
  -DOAML_LIBRARY="/usr/local/lib/liboaml.dylib" \
  -DOAML_INCLUDE_DIR="/usr/local/include" \
  -DOGG_LIBRARY="${BREW_PREFIX}/lib/libogg.dylib" \
  -DVORBIS_LIBRARY="${BREW_PREFIX}/lib/libvorbis.dylib" \
  -DVORBISFILE_LIBRARY="${BREW_PREFIX}/lib/libvorbisfile.dylib"

echo "Building music-tester..."
cmake --build build --target music-tester -j$(sysctl -n hw.ncpu)

echo "Build complete. You can run the music-tester with:"
echo "DYLD_LIBRARY_PATH=/usr/local/lib:${BREW_PREFIX}/lib:${DYLD_LIBRARY_PATH} ./build/music-tester"
echo ""
echo "To clean and rebuild, run this script with the 'clean' argument:"
echo "./music-tester/build-with-libs.sh clean" 