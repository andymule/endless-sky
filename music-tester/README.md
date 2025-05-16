# Music Tester for Endless Sky

A standalone testing application for adaptive music in Endless Sky using OAML (Open Adaptive Music Library) and Dear ImGui.

## Overview

This utility allows music designers to:
- Load audio files (ogg, wav, aif) from a directory
- Control volume for individual tracks
- Add effects to tracks
- Test how different stems work together in an adaptive music context

## System Requirements

- macOS (the build script is optimized for macOS)
- CMake 3.19 or higher
- C++20 compatible compiler
- OAML library installed on your system (`/usr/local/lib/liboaml.dylib`)
- Other dependencies (SDL2, OpenGL, etc.) are handled by the build script

## Installing OAML

The OAML library must be installed on your system. The easiest way is to install from source:

```bash
git clone https://github.com/oamldev/oaml.git
cd oaml
mkdir build && cd build
cmake .. -DENABLE_SHARED=ON -DENABLE_STATIC=ON
make
sudo make install
```

This will install OAML to `/usr/local/lib/` and `/usr/local/include/`.

Verify the installation with:
```bash
ls -la /usr/local/lib/liboaml*
```

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

## OAML Integration

The music-tester uses a simple XML configuration file (`music-tester.defs`) to set up the OAML audio engine. This file is automatically copied to the build directory when the application is built.

## Troubleshooting

If you encounter build problems:

1. Verify that OAML is properly installed:
   ```bash
   ls -la /usr/local/lib/liboaml*
   ```

2. Make sure all required packages are installed through Homebrew:
   ```bash
   brew install libogg libvorbis sdl2 libpng jpeg openal-soft pkg-config
   ```

3. If you're having issues with the build finding dependencies, check that the environment variables are set correctly in `build-macos.sh`.

## Contributing

This tool is designed as a prototype for testing adaptive music concepts that could be integrated into Endless Sky. Feel free to extend it to match your workflow needs.