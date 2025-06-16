# Music Tester for Endless Sky

A standalone testing application for adaptive music in Endless Sky using Dear ImGui.

## Overview

This utility allows music designers to:
- Load audio files (ogg, wav) from a directory
- Control volume for individual tracks
- Add effects to tracks
- Test how different stems work together in an adaptive music context

## System Requirements

- macOS (the build script is optimized for macOS)
- CMake 3.19 or higher
- C++20 compatible compiler
- Other dependencies (SDL2, OpenGL, etc.) are handled by the build script

## Building Music Tester

The music-tester is a standalone application that does not require building the full Endless Sky game.

### Using the Build Script (Recommended)

```bash
cd music-tester
./build-macos.sh
```

This script will:
1. Check for required system libraries (libogg, libvorbis, pkg-config, sdl2, etc.)
2. Install any missing dependencies via Homebrew
3. Use the parent project's vcpkg to install minizip if available, or set up its own
4. Configure and build the music-tester application within the `music-tester/build` directory

If you need to clean and rebuild:
```bash
cd music-tester
./build-macos.sh clean
```

## Running the Application

After building, run the application with:
```bash
cd music-tester
./run-music-tester.sh [optional_music_directory]
```

By default, it will look for audio files in the `sound_staging/` directory.

## Troubleshooting

If you encounter build problems:

1. Make sure all required packages are installed through Homebrew:
   ```bash
   brew install libogg libvorbis sdl2 libpng jpeg openal-soft pkg-config
   ```

2. If you're having issues with the build finding dependencies, check that the environment variables are set correctly in `build-macos.sh`.

## Contributing

This tool is designed as a prototype for testing adaptive music concepts that could be integrated into Endless Sky. Feel free to extend it to match your workflow needs.