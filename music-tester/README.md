# Music Tester for Endless Sky

A standalone testing application for adaptive music in Endless Sky using Dear ImGui with granular synthesis tempo control.

## Overview

This utility allows music designers to:
- Load audio files (ogg, wav, aif) from a directory
- Control volume and effects for individual tracks
- Master tempo control (0.1x to 2.0x speed) with real-time granular synthesis
- Test how different stems work together in an adaptive music context
- Real-time tempo stretching while maintaining original pitch

## Features

### UI Windows
- **Main Window**: Track controls, volume sliders, effects
- **Controls Window**: Keyboard shortcuts and master tempo control
- **Bus Controls**: Master bus effects and volume

### Keyboard Shortcuts
- **Space**: Toggle Play/Pause
- **1-9**: Toggle tracks 1-9 (active window)
- **0**: Toggle track 10 (active window)

### Master Tempo Control
- Real-time tempo stretching using Signalsmith Stretch
- Range: 0.1x to 2.0x playback speed
- Maintains original pitch
- Low latency (~20-40ms)

## System Requirements

- macOS (the build script is optimized for macOS)
- CMake 3.19 or higher
- C++20 compatible compiler
- Ninja build system (optional but recommended)
- Other dependencies (SDL2, OpenGL, etc.) are handled by the build script

## Building Music Tester

The music-tester is a standalone application that does not require building the full Endless Sky game.

### Quick Start

```bash
cd music-tester
./build-macos.sh
./run-music-tester.sh
```

### Build Options

#### Full Build (Recommended for first time)
```bash
./build-macos.sh [options]
```

Options:
- `clean` - Clean build directory before building
- `debug` - Build in Debug mode (default: Release)
- `release` - Build in Release mode

Examples:
```bash
./build-macos.sh clean          # Clean and rebuild
./build-macos.sh debug          # Build in debug mode
./build-macos.sh clean debug    # Clean and build in debug mode
```

#### Quick Build (for development)
For faster iterations after the initial build:
```bash
./quick-build.sh
```

This only runs ninja without dependency checks, perfect for code changes.

## VSCode Integration

This project includes full VSCode integration with:

### Required Extensions
- **C/C++** by Microsoft - `ms-vscode.cpptools` (for IntelliSense and basic C++ support)
- **CodeLLDB** by Vadim Chugunov - `vadimcn.vscode-lldb` (for debugging with modern LLDB)
- **CMake Tools** by Microsoft - `ms-vscode.cmake-tools` (optional, for CMake integration)

Quick install command:
```bash
code --install-extension vadimcn.vscode-lldb
```

### Tasks (Ctrl/Cmd+Shift+P → "Tasks: Run Task")
- **Build Music Tester (Release)** - Full release build
- **Build Music Tester (Debug)** - Debug build
- **Quick Build (Ninja Only)** - Fast incremental build
- **Clean Build** - Clean and rebuild
- **Run Music Tester** - Run with script
- **Build and Run** - Quick build + run (default test task)

### Launch Configurations (F5 or Run menu)
- **Debug Music Tester** - Debug with breakpoints
- **Run Music Tester (No Debug)** - Release mode run
- **Run Music Tester (Script)** - Run via shell script

### IntelliSense
- Full C++20 IntelliSense support
- Includes third-party libraries (Signalsmith, ImGui, SoLoud)
- Auto-completion for all project dependencies

## Running the Application

After building, run the application with:
```bash
./run-music-tester.sh [optional_music_directory]
```

By default, it looks for audio files in the `sound_staging/` directory.

## Third-Party Libraries

This project uses:
- **SoLoud** - Audio engine
- **Dear ImGui** - Immediate mode GUI
- **Signalsmith Stretch** - Real-time tempo stretching
- **SDL2** - Cross-platform multimedia
- **minizip** - Archive handling

## Architecture

```
src/
├── main.cpp              # Application entry point
├── TesterView.h/.cpp     # UI rendering and window management
├── AudioController.h/.cpp # Business logic and state management
├── AudioSystem.h/.cpp    # SoLoud audio system wrapper
├── MasterTempoProcessor.h/.cpp # Granular synthesis tempo control
├── AudioState.h          # Application state structures
└── ErrorHandling.h       # Error handling utilities

third-party/
├── signalsmith-stretch/  # Tempo stretching library
└── signalsmith-linear/   # Linear algebra for audio processing
```

## Troubleshooting

### Build Issues

1. Install required packages:
   ```bash
   brew install libogg libvorbis sdl2 libpng jpeg openal-soft pkg-config
   ```

2. Clean and rebuild:
   ```bash
   ./build-macos.sh clean
   ```

### VSCode Issues

1. Reload window after first build: Cmd+Shift+P → "Developer: Reload Window"
2. Check that compile_commands.json exists in build/
3. Verify C++ extension is using the correct configuration

### Debugging Issues

1. **LLDB MI Interface Error** (`error: unknown option: --interpreter=mi`):
   - Modern LLDB doesn't support the MI interface that VSCode's C++ extension expects
   - **Solution**: Install the CodeLLDB extension:
     - Press `Cmd+Shift+X` in VSCode
     - Search for "CodeLLDB" by Vadim Chugunov
     - Extension ID: `vadimcn.vscode-lldb`
     - Or install via command line: `code --install-extension vadimcn.vscode-lldb`
     - Use "Debug Music Tester" configuration (default)

2. **Alternative Debugging** (if CodeLLDB isn't available):
   - Use "Debug Music Tester (Legacy cppdbg)" configuration
   - This uses the system LLDB with compatibility mode

3. **Xcode Command Line Tools**: Ensure tools are properly linked:
   ```bash
   sudo xcode-select -s /Applications/Xcode250Luck.app/Contents/Developer
   ```

### Audio Issues

1. Ensure audio files are in supported formats (ogg, wav, aif)
2. Check that OpenAL is properly installed
3. Verify audio device permissions in macOS

## Contributing

This tool is designed as a prototype for testing adaptive music concepts that could be integrated into Endless Sky. Feel free to extend it to match your workflow needs.

### Development Workflow

1. Make code changes
2. Run `./quick-build.sh` or use VSCode task "Quick Build"
3. Test with `./run-music-tester.sh` or F5 in VSCode
4. For major changes, use `./build-macos.sh clean` to ensure clean build