# Dynamix for Endless Sky

A standalone testing application for adaptive music in Endless Sky using Dear ImGui with granular synthesis tempo control and event-driven song format support.

## Overview

This utility allows music designers to:
- Load audio files (ogg, wav, aif) from a directory
- Control volume and effects for individual tracks
- Master tempo control (0.1x to 2.0x speed) with real-time granular synthesis
- Test how different stems work together in an adaptive music context
- Real-time tempo stretching while maintaining original pitch
- **NEW**: Event-driven song format with JSON metadata and external triggering

## Features

### UI Windows
- **Main Window**: Track controls, volume sliders, effects
- **Controls Window**: Keyboard shortcuts and master tempo control
- **Bus Controls**: Master bus effects and volume
- **Events Window**: Song events and master bus events (NEW)

### Keyboard Shortcuts
- **Space**: Toggle Play/Pause
- **1-9**: Toggle tracks 1-9 (active window)
- **0**: Toggle track 10 (active window)

### Master Tempo Control
- Real-time tempo stretching using Signalsmith Stretch
- Range: 0.1x to 2.0x playback speed
- Maintains original pitch
- Low latency (~20-40ms)

### Event-Driven Song Format (NEW)
- Songs stored as folders with OGG tracks + JSON metadata
- External event triggering for game engine integration
- Smooth state transitions with lerping
- Complete effect automation for all SoLoud filters

## System Requirements

- **macOS 10.15 or higher** (the build script is optimized for macOS)
- **Xcode Command Line Tools** (required for C++ compilation)
- **Homebrew** (for dependency management)
- **CMake 3.24 or higher** (required by Signalsmith libraries)
- **C++20 compatible compiler** (Clang 12+ or GCC 10+)
- **Ninja build system** (optional but recommended)

## Fresh Computer Setup

### 1. Install Xcode Command Line Tools

First, install the Xcode Command Line Tools which provide the C++ compiler:

```bash
xcode-select --install
```

Wait for the installation to complete (this may take several minutes).

### 2. Install Homebrew

Install Homebrew, the package manager for macOS:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

After installation, add Homebrew to your PATH (follow the instructions shown in the terminal).

### 3. Install Required Dependencies

Install all required system libraries:

```bash
# Core development tools
brew install cmake ninja pkg-config ccache

# Audio and multimedia libraries
brew install sdl2 libpng jpeg openal-soft

# Audio codec support
brew install libogg libvorbis

# Git (if not already installed)
brew install git
```

### 4. Clone the Repository

Clone the dynamix repository:

```bash
git clone https://github.com/your-username/dynamix.git
cd dynamix
```

### 5. Build the Application

Run the build script which will automatically:
- Download and configure all third-party libraries
- Build the application with optimal settings
- Copy required assets to the build directory

```bash
./build-macos.sh
```

### 6. Run the Application

After successful build, run the application:

```bash
cd build && ./dynamix
```

## Building Dynamix

The dynamix is a standalone application that does not require building the full Endless Sky game.

### Quick Start

```bash
cd dynamix
./build-macos.sh
cd build && ./dynamix
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
- `static` - Build with static linking (standalone binary)

Examples:
```bash
./build-macos.sh clean          # Clean and rebuild
./build-macos.sh debug          # Build in debug mode
./build-macos.sh clean debug    # Clean and build in debug mode
./build-macos.sh static         # Build standalone binary
./build-macos.sh clean static   # Clean and build standalone binary
```

#### Quick Build (for development)
For faster iterations after the initial build:
```bash
./quick-build.sh
```

This only runs ninja without dependency checks, perfect for code changes.

### Build Performance Optimization

This project is optimized for fast development cycles:

- **ccache**: Automatically caches compiled objects for 2x-10x faster rebuilds
- **Unity builds**: Combines multiple source files to reduce compilation overhead
- **Pre-downloaded dependencies**: Dependencies are cached locally for instant configuration
- **Ninja build system**: Highly parallel builds with minimal overhead

For maximum build performance:
1. Install ccache: `brew install ccache` (already included in dependencies above)
2. Use incremental builds: `ninja` or `./quick-build.sh` for code changes
3. See `build-times.md` for detailed optimization strategies

## Binary Distribution

### Bundled Binary (Recommended for Distribution)

To create a self-contained binary that bundles SDL2 with the application:

```bash
./build-macos.sh bundle
```

This creates a binary with SDL2 bundled alongside it, making it portable to other macOS systems without requiring SDL2 installation.

**What's bundled:**
- ✅ SDL2 dylib (bundled with @rpath)
- ✅ SoLoud, ImGui, Signalsmith libraries (statically linked)
- ✅ All application code and assets

**What's still dynamically linked (system dependencies):**
- System frameworks: OpenAL, OpenGL, Accelerate, CoreGraphics, etc.
- System libraries: libc++, libSystem, etc.

This approach follows macOS best practices by:
- Bundling third-party libraries (SDL2) to avoid dependency issues
- Using system frameworks dynamically (reduces binary size, ensures compatibility)
- Creating a portable binary that works on any macOS 10.15+ system

### Running a Pre-built Binary

If you have received a pre-built binary:

#### Bundled Binary (Recommended)
```bash
# Simply run the binary - SDL2 is bundled
./dynamix
```

**System requirements:**
- macOS 10.15 or higher
- Standard system frameworks (always available on macOS)
- No additional library installation required

#### Dynamic Binary (Legacy)
If the binary was built without bundling, you'll need to install SDL2:

```bash
# Install SDL2 via Homebrew
brew install sdl2

# Then run the binary
./dynamix
```

### Binary Distribution Package

For easy distribution, you can create a complete package:

```bash
# Build bundled binary
./build-macos.sh bundle

# Create distribution package
mkdir dynamix-dist
cp build/dynamix dynamix-dist/
cp build/libSDL2-2.0.0.dylib dynamix-dist/  # Bundled SDL2
cp -r sound_staging dynamix-dist/
cp README.md dynamix-dist/

# Create a simple run script
cat > dynamix-dist/run.sh << 'EOF'
#!/bin/bash
cd "$(dirname "$0")"
./dynamix
EOF
chmod +x dynamix-dist/run.sh

# Archive the package
tar -czf dynamix-macos.tar.gz dynamix-dist/
```

### System Requirements for Binary Distribution

#### Bundled Binary (Recommended)
- **macOS 10.15 or higher** (Catalina+)
- **Standard system frameworks** (OpenAL, OpenGL, Accelerate, CoreGraphics, etc.)
- **No additional library installation required** (SDL2 is bundled, our libraries are statically linked)

#### Dynamic Binary (Legacy)
- **macOS 10.15 or higher** (Catalina+)
- **Homebrew** (for dependency installation)
- **SDL2 library**: `brew install sdl2`

### Troubleshooting Binary Issues

#### "Library not found" Errors
If you get library not found errors when running a dynamic binary:

1. **Install missing libraries**:
   ```bash
   brew install sdl2
   ```

2. **Check library paths**:
   ```bash
   otool -L dynamix
   ```

3. **Use bundled binary** instead:
   ```bash
   ./build-macos.sh bundle
   ```

#### "SDL2 not found" Errors
If you get SDL2 not found errors:
- **For bundled binaries**: The SDL2 dylib should be in the same directory as the binary
- **For dynamic binaries**: Install SDL2: `brew install sdl2`
- **Check the binary type**: `otool -L dynamix` should show `@rpath/libSDL2-2.0.0.dylib` for bundled builds

#### "Permission denied" Errors
```bash
chmod +x dynamix
```

#### "Audio device not found" Errors
- Check that your audio output device is working
- Verify audio permissions in macOS System Preferences
- Try running with different audio settings

#### "File not found" Errors
- Ensure the `sound_staging/` directory exists in the same folder as the binary
- Check that audio files are in supported formats (OGG, WAV, AIF)

#### "Framework not found" Errors
If you get errors about missing system frameworks (OpenAL, OpenGL, etc.):
- These frameworks are part of macOS and should always be available
- Try updating macOS to the latest version
- Check that you're running on a supported macOS version (10.15+)

## Event-Driven Song Format

The application now supports an event-driven song format for adaptive music:

### File Structure
```
sound_staging/
├── _master.json           # Master bus events (global effects)
├── song-name-1/
│   ├── _song.json        # Song metadata + events
│   ├── drums.ogg         # Track files (OGG only)
│   ├── bass.ogg
│   └── melody.ogg
└── song-name-2/
    ├── _song.json
    └── track1.ogg
```

### External API
Game engines can trigger events via the C-style API:
```cpp
// Trigger song events (affects tracks + per-song tempo)
dynamix_triggerSongEvent("battle-theme", "intense");
dynamix_triggerSongEvent("ambient-forest", "night-cycle");

// Trigger master bus events (affects global effects + master tempo)
dynamix_triggerMasterEvent("underwater");
dynamix_triggerMasterEvent("normal");
```

### JSON Format Example
```json
{
  "name": "Test Song",
  "events": [
    {
      "name": "Intro",
      "fadeTime": 2.0,
      "state": {
        "masterTempo": 1.0,
        "granularTempo": 1.0,
        "tracks": [
          {
            "file": "drums.ogg",
            "volume": 0.8,
            "active": true,
            "effects": {
              "BassBoost": {
                "enabled": true,
                "parameters": {
                  "boost": 3.0
                }
              }
            }
          }
        ]
      }
    }
  ]
}
```

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
- **Build Dynamix (Release)** - Full release build
- **Build Dynamix (Debug)** - Debug build
- **Quick Build (Ninja Only)** - Fast incremental build
- **Clean Build** - Clean and rebuild
- **Run Dynamix** - Run with script
- **Build and Run** - Quick build + run (default test task)

### Launch Configurations (F5 or Run menu)
- **Debug Dynamix** - Debug with breakpoints
- **Run Dynamix (No Debug)** - Release mode run
- **Run Dynamix (Script)** - Run via shell script

### IntelliSense
- Full C++20 IntelliSense support
- Includes third-party libraries (Signalsmith, ImGui, SoLoud)
- Auto-completion for all project dependencies

## Running the Application

After building, run the application with:
```bash
cd build && ./dynamix
```

By default, it looks for audio files in the `sound_staging/` directory.

## Third-Party Libraries

This project uses:
- **SoLoud** - Audio engine
- **Dear ImGui** - Immediate mode GUI
- **Signalsmith Stretch** - Real-time tempo stretching
- **SDL2** - Cross-platform multimedia
- **minizip** - Archive handling
- **nlohmann/json** - JSON parsing (NEW)

## Architecture

```
src/
├── main.cpp              # Application entry point
├── TesterView.h/.cpp     # UI rendering and window management
├── AudioController.h/.cpp # Business logic and state management
├── AudioSystem.h/.cpp    # SoLoud audio system wrapper
├── AudioStreamProcessor.h/.cpp # Granular synthesis tempo control
├── AudioState.h          # Application state structures
├── SongManager.h/.cpp    # Event-driven song loading (NEW)
├── EventSystem.h/.cpp    # Event triggering and transitions (NEW)
└── ErrorHandling.h       # Error handling utilities

Dependencies (managed by CMake FetchContent):
├── ImGui                 # Immediate mode GUI framework
├── SoLoud                # Audio engine
├── Signalsmith Stretch   # Real-time tempo stretching
├── Signalsmith Linear    # Linear algebra for audio processing
└── nlohmann/json         # JSON parsing library
```

## Troubleshooting

### Build Issues

1. **Missing Dependencies**: Ensure all required packages are installed:
   ```bash
   brew install cmake ninja pkg-config sdl2 libpng jpeg openal-soft libogg libvorbis
   ```

2. **Xcode Command Line Tools**: Verify installation:
   ```bash
   xcode-select --print-path
   # Should show: /Applications/Xcode.app/Contents/Developer
   ```

3. **Clean and rebuild**:
   ```bash
   ./build-macos.sh clean
   ```

4. **Permission Issues**: If you get permission errors:
   ```bash
   chmod +x build-macos.sh
   chmod +x quick-build.sh
   ```

### VSCode Issues

1. **Reload window** after first build: Cmd+Shift+P → "Developer: Reload Window"
2. **Check compile_commands.json** exists in build/
3. **Verify C++ extension** is using the correct configuration

### Debugging Issues

1. **LLDB MI Interface Error** (`error: unknown option: --interpreter=mi`):
   - Modern LLDB doesn't support the MI interface that VSCode's C++ extension expects
   - **Solution**: Install the CodeLLDB extension:
     - Press `Cmd+Shift+X` in VSCode
     - Search for "CodeLLDB" by Vadim Chugunov
     - Extension ID: `vadimcn.vscode-lldb`
     - Or install via command line: `code --install-extension vadimcn.vscode-lldb`
     - Use "Debug Dynamix" configuration (default)

2. **Alternative Debugging** (if CodeLLDB isn't available):
   - Use "Debug Dynamix (Legacy cppdbg)" configuration
   - This uses the system LLDB with compatibility mode

3. **Xcode Command Line Tools**: Ensure tools are properly linked:
   ```bash
   sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
   ```

### Audio Issues

1. **Supported Formats**: Ensure audio files are in supported formats (ogg, wav, aif)
2. **OpenAL**: Check that OpenAL is properly installed:
   ```bash
   brew install openal-soft
   ```
3. **Audio Permissions**: Verify audio device permissions in macOS System Preferences
4. **Audio Device**: Check that your audio output device is working and not muted

### Event System Issues

1. **JSON Format**: Ensure JSON files follow the correct format (see examples above)
2. **File Paths**: Check that track filenames in JSON match actual files in the song folder
3. **OGG Files**: Only OGG files are supported for tracks (as per specification)
4. **Event Names**: Ensure event names match exactly between JSON and API calls

## Contributing

This tool is designed as a prototype for testing adaptive music concepts that could be integrated into Endless Sky. Feel free to extend it to match your workflow needs.

### Development Workflow

1. Make code changes
2. Test with quick build: `./quick-build.sh`
3. Commit changes with descriptive messages

## License

This project is part of the Endless Sky ecosystem and follows the same licensing terms.