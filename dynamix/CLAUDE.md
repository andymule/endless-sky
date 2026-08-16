# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Dynamix is a standalone real-time audio testing application for adaptive music in Endless Sky. It provides granular synthesis tempo control, event-driven song format support, and an ImGui-based interface for testing how different stems work together in an adaptive music context.

## Common Development Commands

### Building
- **Windows (this repo's primary day-to-day)**: `.\build-windows.ps1 -Config Release` or `build-windows-fast.bat`
- **Windows tests**: `.\run-tests.bat` or `.\build-windows.ps1 -Config Release -Tests`
- **macOS full build**: `./build-macos.sh`
- **Linux**: `./build-linux.sh`
- **Quick incremental (Unix)**: `./quick-build.sh` or `ninja` in the build directory

### Running
- **Windows**: `run-dynamix.bat` or `build\dynamix.exe`
- **Unix**: `cd build && ./dynamix`

### Build System
- Uses **CMake** with **Ninja** build system for performance
- **Unity builds** enabled by default for faster compilation
- **ccache** automatically used if available for 2x-10x faster rebuilds
- **Dependencies managed via CMake FetchContent** (optional persistent `.deps/` on macOS)
- **C++20** standard required
- Windows system package: **SDL2** (plus cmake/ninja/clang). PNG/JPEG/OpenAL/GLEW/minizip are not required.

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
├── JsonValidator.h/.cpp       # Song/master JSON validation
├── ConsoleLog.h/.cpp          # In-app log drawer
└── Views/                     # Additional UI components
```

### Audio File Format
- **Songs**: Directories containing `_song.json` + OGG tracks
- **Master Events**: `_master.json` for global effects
- **OGG for tracks**; the validator also accepts `.wav`, `.aif`, `.aiff`
- **Event-driven**: JSON defines events that trigger state changes
- **Forgiving loader**: only structurally broken JSON fails; missing tracks, unknown effects, and out-of-range values become console warnings (see `fileformat.md`)

## Development Workflow

### Making Code Changes
1. Edit source files
2. Windows: `.\build-windows.ps1 -Config Release` (or Ctrl+Shift+B)
3. Test with `.\run-tests.bat` (`[unit]` tier) or `.\build\dynamix_tests.exe` (full suite), then `.\run-dynamix.bat`

The full suite mixes real audio through SoLoud's null driver, so it needs no audio device. Effect tests attach filters to a track through `FilterManager`; because a SoLoud voice only creates filter instances when it starts, attaching a filter to a playing track restarts it. Frequency analysis in `SignalAnalyzer` treats its input as one stream, so pass `extractChannel()` output rather than an interleaved capture.

Unix: `./quick-build.sh` then `cd build && ./dynamix`

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

Day-to-day development is on **Windows** (MSYS2 MinGW64 + Clang + gdb). macOS tasks remain for the other machine.

### Required Extensions
- **C/C++** (`ms-vscode.cpptools`) - IntelliSense and Windows gdb debugging

### Available Tasks (Ctrl+Shift+B / Tasks: Run Task)
- **Build Dynamix (Release - Windows)** - default build via `build-windows.ps1`
- **Build Dynamix (Debug - Windows)** - debug build to `build_debug/`
- **Build and Test (Windows)** - Release + Catch2 `[unit]`
- **Run Dynamix (Windows)** - launch `build/dynamix.exe`
- Unix tasks (`Build Dynamix (* - Unix)`) still wrap `./build-macos.sh`

### Launch Configurations (F5)
- **Debug Dynamix (Windows gdb)** - `build_debug/dynamix.exe` with MinGW gdb
- **Run Dynamix (No Debug)** - Release binary

Unix/macOS: `./build-macos.sh` then `cd build && ./dynamix`. CodeLLDB is optional on Mac.

## Dependencies and System Requirements

### System Libraries (Windows / MSYS2)
- `mingw-w64-x86_64-{cmake,ninja,clang,pkgconf,SDL2,gdb}` via `install-msys2-deps.bat`

### System Libraries (macOS / Homebrew)
- cmake, ninja, pkg-config, ccache, sdl2

### Third-Party Libraries (auto-downloaded)
- **SoLoud** - Audio engine
- **Dear ImGui** - GUI framework
- **Signalsmith Stretch** - Real-time tempo stretching
- **nlohmann/json** - JSON parsing

### Platform Support
- **Primary day-to-day**: Windows 10/11 with MSYS2 MinGW64 (Clang + Ninja + SDL2). See `how-to-windows.md`.
- **Also**: macOS 10.15+ with Xcode Command Line Tools; Linux via `./build-linux.sh`
- **Build system**: CMake 3.24+, Ninja
- **Compiler**: C++20 compatible (Clang 12+, GCC 10+)

## Troubleshooting

### Build Issues
- Windows: `.\build-windows.ps1 -Config Release -Clean`, then `install-msys2-deps.bat` if cmake/clang/SDL2 are missing
- Unix: `./build-macos.sh clean`; Homebrew: `brew install cmake ninja pkg-config sdl2`

### Audio Issues
- Tracks are OGG in practice; the validator also accepts `.wav`, `.aif`, `.aiff`
- Projects live in `~/Music/Dynamix` by default (Windows: `%USERPROFILE%\Music\Dynamix`); a project folder needs `_master.json` and each song folder needs `_song.json`
- Loading problems are reported in the in-app console: most are warnings the loader recovers from (see `fileformat.md`)
- Verify audio device permissions in macOS System Preferences

### VSCode/Debugging Issues
- Install the C/C++ extension (`ms-vscode.cpptools`) and `mingw-w64-x86_64-gdb`
- Reload window after first build for IntelliSense (`build/compile_commands.json`)