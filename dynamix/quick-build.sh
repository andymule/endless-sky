#!/bin/bash
set -e

# Quick build script for development - only runs ninja
# Use this after you've done a full build once with build-macos.sh

BUILD_DIR="$(dirname "$0")/build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Run './build-macos.sh' first."
    exit 1
fi

if [ ! -f "$BUILD_DIR/build.ninja" ]; then
    echo "Ninja files not found. Run './build-macos.sh' first."
    exit 1
fi

echo "Quick building with ninja..."
cd "$BUILD_DIR"
ninja

echo "Quick build complete!"
echo ""
echo "You can run the dynamix with:"
echo "cd build && ./dynamix" 