#!/bin/bash
set -e

# Print header
echo "=== SoLoud Test macOS Build Script ==="
echo ""

# Get Homebrew SDL2 paths
SDL2_PREFIX=$(brew --prefix sdl2)
if [ -z "$SDL2_PREFIX" ]; then
    echo "Error: SDL2 not found in Homebrew. Install with: brew install sdl2"
    exit 1
fi

echo "Found SDL2 at: $SDL2_PREFIX"

# Set environment variables to help CMake find SDL2
export PKG_CONFIG_PATH="$SDL2_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH"
export CMAKE_PREFIX_PATH="$SDL2_PREFIX:$CMAKE_PREFIX_PATH"
export SDL2_DIR="$SDL2_PREFIX"

# For compiler flags
export CFLAGS="-I$SDL2_PREFIX/include/SDL2"
export CXXFLAGS="-I$SDL2_PREFIX/include/SDL2"

# Display environment for debugging
echo "PKG_CONFIG_PATH: $PKG_CONFIG_PATH"
echo "CMAKE_PREFIX_PATH: $CMAKE_PREFIX_PATH"
echo "SDL2_DIR: $SDL2_DIR"
echo "CFLAGS: $CFLAGS"
echo "CXXFLAGS: $CXXFLAGS"

# Determine the number of jobs to use for faster compilation
JOBS=$(sysctl -n hw.logicalcpu)
echo "Using $JOBS parallel jobs"

# Working directory
SCRIPT_DIR="$(dirname "$0")"
cd "$SCRIPT_DIR"
echo "Working directory: $(pwd)"

# Make sure the directory is correct
[ -f "CMakeLists.txt" ] || { echo "Error: CMakeLists.txt not found! Wrong directory?"; exit 1; }
[ -d "../extern/soloud" ] || { echo "Error: SoLoud directory not found at ../extern/soloud"; exit 1; }

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake - explicitly set include paths and library paths
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DSDL2_DIR="$SDL2_PREFIX" \
    -DSDL2_INCLUDE_DIR="$SDL2_PREFIX/include/SDL2" \
    -DSDL2_LIBRARY="$SDL2_PREFIX/lib/libSDL2.dylib" \
    -DCMAKE_CXX_FLAGS="-I$SDL2_PREFIX/include/SDL2" \
    -DCMAKE_C_FLAGS="-I$SDL2_PREFIX/include/SDL2"

# Show include paths for debugging
echo "C++ Compiler: $(which $(cmake -LA -N . | grep CMAKE_CXX_COMPILER | cut -d= -f2))"
echo "Include paths:"
echo "$CXXFLAGS"

# Build with verbose output to see all compiler commands
echo "Compiling with $JOBS parallel jobs..."
VERBOSE=1 cmake --build . -j "$JOBS"

# Check if build was successful
if [ $? -eq 0 ]; then
    echo ""
    echo "Build completed successfully!"
    echo "You can run the SoLoud test with:"
    echo "./soloud_test"
    echo ""
else
    echo ""
    echo "Build failed! See error messages above."
    exit 1
fi 