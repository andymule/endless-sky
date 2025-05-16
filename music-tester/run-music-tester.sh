#!/bin/bash
set -e

# Get Homebrew prefix
BREW_PREFIX=$(brew --prefix)

# Set library paths
export DYLD_LIBRARY_PATH="/usr/local/lib:${BREW_PREFIX}/lib:${DYLD_LIBRARY_PATH}"

# Get the script directory and build directory
SCRIPT_DIR="$(dirname "$0")"
cd "$SCRIPT_DIR"
BUILD_DIR="$(pwd)/build"

echo "Running music-tester with DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH"

# Pass any command line arguments to the music-tester
"$BUILD_DIR/music-tester" "$@" 