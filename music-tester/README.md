# Music Tester for Endless Sky

A testing application for adaptive music in Endless Sky using OAML (Open Adaptive Music Library) and Dear ImGui.

## Overview

This utility allows music designers to:
- Load audio files (ogg, wav, aif) from a directory
- Control volume for individual tracks
- Add effects to tracks
- Test how different stems work together in an adaptive music context

## System Requirements

- OAML library installed on your system (`/usr/local/lib/liboaml.dylib`)
- SDL2, OpenGL, and other dependencies (handled by the build scripts)

## Quick Start

### Using Build Scripts (Recommended)

For macOS:
```bash
./music-tester/build-macos.sh
```

This script automatically handles dependency issues and builds the application.

### Manual Build

1. Ensure OAML is installed on your system:
   ```
   ls -la /usr/local/lib/liboaml*
   ```

2. Build the `music-tester` target from the Endless Sky project:
   ```
   cmake -B build -DES_BUILD_MUSIC_TESTER=ON
   cmake --build build --target music-tester
   ```

3. Run the application:
   ```
   ./build/music-tester [optional_music_directory_path]
   ```

   If no directory is specified, it will try to use `sound_staging/` by default.

### Build Issues?

If you encounter build problems, see the detailed instructions in:
- [BUILD-INSTRUCTIONS.md](BUILD-INSTRUCTIONS.md) for general build guidance
- [README-SYSTEM-OAML.md](README-SYSTEM-OAML.md) for OAML-specific setup

## Usage

In the application:
- Enter a directory path in the text field and press Enter or click "Load" to load audio files
- Use the checkboxes to enable or disable tracks
- Adjust volume sliders to control the mix
- Add effects using the dropdown and customize their parameters

## OAML Integration

The music-tester uses the Open Adaptive Music Library (OAML) to handle audio playback and adaptive music features. The configuration is stored in `music-tester.defs` which can be edited to define adaptive track behaviors.

## Contributing

This tool is designed as a prototype for testing adaptive music concepts that could be integrated into Endless Sky. Feel free to extend it to match your workflow needs.