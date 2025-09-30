# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Dynamix is a standalone real-time audio testing application for adaptive music in Endless Sky. It provides granular synthesis tempo control, event-driven song format support, and an ImGui-based interface for testing how different stems work together in an adaptive music context.

## Common Development Commands

### Building
- **Full build (first time)**: `./build-macos.sh`
- **Quick incremental build**: `./quick-build.sh` or `ninja` in build directory
- **Clean rebuild**: `./build-macos.sh clean`
- **Debug build**: `./build-macos.sh debug`
- **Bundled binary (portable)**: `./build-macos.sh bundle`
- **Static build**: `./build-macos.sh static`

### Running
- **Run application**: `cd build && ./dynamix`

### Build System
- Uses **CMake** with **Ninja** build system for performance
- **Unity builds** enabled by default for faster compilation
- **ccache** automatically used if available for 2x-10x faster rebuilds
- **C++20** standard required
- Dependencies managed via **CMake FetchContent** and **vcpkg** for minizip

## Code Architecture

### MVC Pattern
The codebase follows a Model-View-Controller architecture:
- **Model**: `AudioState.h`, `AudioSystem.cpp`, `SongManager.cpp` - Data structures and audio processing
- **View**: `MainView.cpp` (formerly TesterView) - Dear ImGui interface
- **Controller**: `AudioController.cpp` - Business logic coordination

### Key Components

#### Core Audio System
- **AudioSystem**: SoLoud audio engine wrapper with custom filters and sync
- **AudioStreamProcessor**: Real-time granular time stretching using Signalsmith Stretch
- **AudioController**: Central coordinator managing playback, effects, and state

#### Event-Driven Music System
- **SongManager**: Loads songs from JSON + OGG files, manages events
- **EventSystem**: Smooth state transitions with lerping and easing curves
- **Event format**: Songs stored as directories with `_song.json` + OGG tracks

#### Audio Processing
- **Dual Tape Speed Architecture**: Internal tape speed = userTapeSpeed * granularTempo
- **FilterManager**: Unified effect parameter management for all SoLoud filters
- **CircularBuffer**: Lock-free ring buffer for real-time audio processing

### File Structure
```
src/
├── main.cpp                    # Application entry + external C API
├── AudioController.h/.cpp      # MVC controller and business logic
├── AudioSystem.h/.cpp          # SoLoud wrapper and audio processing
├── AudioStreamProcessor.h/.cpp # Granular tempo control with Signalsmith
├── AudioState.h                # Data structures and state management
├── MainView.h/.cpp            # Dear ImGui interface (formerly TesterView)
├── SongManager.h/.cpp         # Event-driven song loading and JSON parsing
├── EventSystem.h/.cpp         # State transitions and effect automation
├── FilterManager.h/.cpp       # Audio effect parameter management
├── music_tester_api.h         # External C API definitions
└── Views/                     # Additional UI components
```

### Audio File Format
- **Songs**: Directories containing `_song.json` + OGG tracks
- **Master Events**: `_master.json` for global effects
- **Only OGG files supported** for tracks
- **Event-driven**: JSON defines events that trigger state changes

## Development Workflow

### Making Code Changes
1. Edit source files
2. Run `./quick-build.sh` for fast incremental build
3. Test with `cd build && ./dynamix`

### Adding New Features
- All audio tracks **loop continuously** - use volume=0 to disable
- Effect parameters stored as string IDs for JSON compatibility
- State transitions use **wet level logic** (only effects with wet > 0 are active)
- Use **robust error handling** with graceful degradation

### Key Architectural Principles
- **Lock-free audio processing** using atomic operations and circular buffers
- **Smooth state transitions** with EASE_IN_OUT curves and lerping
- **Parameter validation** with range checking and defaults
- **Graceful error recovery** - continue operation even if some components fail

## VSCode Integration

### Required Extensions
- **C/C++** (ms-vscode.cpptools) - IntelliSense and basic C++ support
- **CodeLLDB** (vadimcn.vscode-lldb) - Debugging with modern LLDB

### Available Tasks (Cmd+Shift+P → "Tasks: Run Task")
- **Build Dynamix (Release - Unix)** - Full release build via `./build-macos.sh`
- **Build Dynamix (Debug - Unix)** - Debug build via `./build-macos.sh debug`
- **Build Dynamix (Fast - Unix)** - Fast debug build via `./build-macos.sh debug`
- **Quick Build (Ninja Only - Unix)** - Fast incremental build via ninja
- **Build and Run (Unix)** - Fast build + run (depends on Fast build + Run tasks)
- **Clean Build** - Clean and rebuild
- **Run Dynamix (No Build - Unix)** - Run without building via `./build/dynamix`

### Launch Configurations (F5)
- **Debug Dynamix** - Debug with breakpoints using CodeLLDB
- **Run Dynamix (No Debug)** - Release mode run

## Dependencies and System Requirements

### System Libraries (via Homebrew)
- cmake, ninja, pkg-config, ccache
- sdl2, libpng, jpeg, openal-soft, libogg, libvorbis

### Third-Party Libraries (auto-downloaded)
- **SoLoud** - Audio engine
- **Dear ImGui** - GUI framework
- **Signalsmith Stretch** - Real-time tempo stretching
- **nlohmann/json** - JSON parsing
- **minizip** - Archive handling (via vcpkg)

### Platform Support
- **Primary**: macOS 10.15+ with Xcode Command Line Tools
- **Build system**: CMake 3.24+, Ninja (optional but recommended)
- **Compiler**: C++20 compatible (Clang 12+, GCC 10+)

## Troubleshooting

### Build Issues
- Run `./build-macos.sh clean` for mysterious build failures
- Ensure Xcode Command Line Tools installed: `xcode-select --install`
- Check Homebrew dependencies: `brew install cmake ninja pkg-config sdl2 openal-soft`

### Audio Issues
- Only OGG/WAV/AIF files supported
- Check `sound_staging/` directory exists with proper format
- Verify audio device permissions in macOS System Preferences

### VSCode/Debugging Issues
- Install CodeLLDB extension for modern debugging
- Reload window after first build for IntelliSense
- Check `build/compile_commands.json` exists for proper IntelliSense