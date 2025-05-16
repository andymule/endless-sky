#!/bin/bash
set -e

# Get Homebrew prefix
BREW_PREFIX=$(brew --prefix)

# Set library paths
export DYLD_LIBRARY_PATH="/usr/local/lib:${BREW_PREFIX}/lib:${DYLD_LIBRARY_PATH}"

# Run the music-tester
cd "$(dirname "$0")/.."
echo "Running music-tester with DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH"

# Pass any command line arguments to the music-tester
./build/music-tester/music-tester "$@" 