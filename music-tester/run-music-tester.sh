#!/bin/bash
set -e

# Get Homebrew prefix
BREW_PREFIX=$(brew --prefix)

# Set library paths
export DYLD_LIBRARY_PATH="${BREW_PREFIX}/lib:${DYLD_LIBRARY_PATH}"

# Get the script directory and build directory
SCRIPT_DIR="$(dirname "$0")"
cd "$SCRIPT_DIR"
MUSIC_TESTER_DIR=$(pwd)
BUILD_DIR="${MUSIC_TESTER_DIR}/build"

# If a directory is provided as an argument, use it, otherwise use current directory
if [ $# -gt 0 ]; then
    SOUND_DIR="$1"
    shift
else
    SOUND_DIR="."
fi

echo "Running music-tester"
echo "Music directory: $SOUND_DIR"

# Pass any remaining command line arguments to the music-tester
"$BUILD_DIR/music-tester" "$SOUND_DIR" "$@" 