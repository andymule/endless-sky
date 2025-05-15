#!/bin/bash

# Default to Release mode unless DEBUG is set
BUILD_TYPE=${DEBUG:+Debug}
BUILD_TYPE=${BUILD_TYPE:-Release}

# Ensure vcpkg dependencies are installed
./vcpkg/vcpkg install --feature-flags=manifests --x-feature=system-libs

# Configure with CMake
cmake -B build \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
  -Dunofficial-minizip_DIR=$(pwd)/vcpkg/packages/minizip_arm64-osx/share/unofficial-minizip \
  -DLIBMAD_INCLUDE_DIR=$(pwd)/vcpkg/packages/libmad_arm64-osx/include \
  -DLIBMAD_LIB_RELEASE=$(pwd)/vcpkg/packages/libmad_arm64-osx/lib/libmad.a \
  -DLIBMAD_LIB_DEBUG=$(pwd)/vcpkg/packages/libmad_arm64-osx/debug/lib/libmad.a

# Build using all available cores
cmake --build build -j$(sysctl -n hw.ncpu) --config $BUILD_TYPE

echo "Build complete. Run with ./build/endless-sky"
echo "Build type: $BUILD_TYPE" 