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
SOUND_DIR="${MUSIC_TESTER_DIR}/sound_staging"

# Create sound_staging directory if it doesn't exist
mkdir -p "$SOUND_DIR"

echo "Running music-tester"
echo "Music directory: $SOUND_DIR"

# Pass any command line arguments to the music-tester
# Add the sound directory as the first argument
"$BUILD_DIR/music-tester" "$SOUND_DIR" "$@" 