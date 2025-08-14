# Fresh Install Setup for Dynamix on macOS

This document outlines the gaps that were identified and fixed for a fresh macOS install to work with the "Build and Run (Fast - macOS/Linux)" launch configuration.

## Issues Found and Fixed

### 1. Missing Core Dependencies
**Problem**: Fresh macOS installs were missing essential build tools.
**Solution**: Updated build script to check for and install:
- CMake (version 3.24+ required)
- Ninja (for faster builds)
- pkg-config
- SDL2
- libpng
- jpeg (keg-only)
- openal-soft (keg-only)

### 2. Keg-only Library Handling
**Problem**: JPEG and OpenAL-Soft are "keg-only" in Homebrew, meaning they're not symlinked into standard paths.
**Solution**: 
- Updated `PKG_CONFIG_PATH` to include keg-only library paths
- Updated `CMAKE_PREFIX_PATH` to include `/opt/homebrew/opt/jpeg` and `/opt/homebrew/opt/openal-soft`

### 3. vcpkg Clone Directory Issue
**Problem**: The vcpkg clone command wasn't specifying the target directory explicitly.
**Solution**: Changed from `git clone --depth=1 https://github.com/microsoft/vcpkg.git` to `git clone --depth=1 https://github.com/microsoft/vcpkg.git vcpkg`

### 4. Debugging vcpkg Detection
**Problem**: Sometimes the parent vcpkg detection would fail silently.
**Solution**: Added debug output to show vcpkg directory detection process.

## Files Modified

### build-macos.sh
1. Fixed vcpkg clone command to specify target directory
2. Added keg-only library paths to PKG_CONFIG_PATH
3. Updated CMAKE_PREFIX_PATH to include keg-only library paths
4. Added cmake and ninja to required packages list
5. Added debug output for vcpkg detection

## New Files Added

### setup-fresh-install.sh
A comprehensive setup script that:
- Checks for and installs Homebrew if needed
- Verifies Xcode Command Line Tools
- Installs all required Homebrew packages
- Sets up vcpkg if needed
- Runs a test build to verify everything works

### .vscode/extensions.json
VS Code workspace extension recommendations file that automatically suggests the CodeLLDB extension when opening the workspace.

## VS Code Configuration

The existing VS Code launch configuration "Build and Run (Fast - macOS/Linux)" now works seamlessly on fresh installs with these fixes.

**Required VS Code Extension**: The launch configurations require the **CodeLLDB** extension for debugging support. Install it from the VS Code marketplace:
- Extension ID: `vadimcn.vscode-lldb`
- Or search for "CodeLLDB" in the VS Code Extensions tab
- VS Code will automatically recommend this extension when you open the workspace (configured in `.vscode/extensions.json`)

The launch configurations use `lldb-dap` and `lldb` debugger types which are provided by this extension.

## Quick Setup for New Developers

For a completely fresh macOS system:

1. Clone the repository
2. Run the setup script: `./setup-fresh-install.sh`
3. Install the **CodeLLDB** VS Code extension (`vadimcn.vscode-lldb`)
4. Use the "Build and Run (Fast - macOS/Linux)" launch configuration in VS Code

## Manual Dependencies (If Needed)

If you prefer to install dependencies manually:

```bash
# Install Homebrew (if not installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install required packages
brew install cmake ninja pkg-config sdl2 libpng jpeg openal-soft

# Install Xcode Command Line Tools (if not installed)
xcode-select --install
```

## Environment Variables Set by build-macos.sh

The build script automatically sets these environment variables for keg-only libraries:

```bash
export PKG_CONFIG_PATH="${BREW_PREFIX}/lib/pkgconfig:${BREW_PREFIX}/opt/libvorbis/lib/pkgconfig:${BREW_PREFIX}/opt/libogg/lib/pkgconfig:${BREW_PREFIX}/opt/libpng/lib/pkgconfig:${BREW_PREFIX}/opt/jpeg/lib/pkgconfig:${BREW_PREFIX}/opt/openal-soft/lib/pkgconfig:/usr/local/lib/pkgconfig:${PKG_CONFIG_PATH}"
```

And passes these paths to CMake:

```bash
-DCMAKE_PREFIX_PATH="${BREW_PREFIX};${BREW_PREFIX}/opt/jpeg;${BREW_PREFIX}/opt/openal-soft;/usr/local"
```

## Verification

The setup is working correctly when:
1. The launch configuration builds and runs without manual intervention
2. The build script finds the parent vcpkg directory
3. All required libraries are detected by CMake
4. The application starts successfully

## Troubleshooting

If you encounter issues:
1. Run `./setup-fresh-install.sh` to verify all dependencies
2. Check the build output for missing packages
3. Ensure Xcode Command Line Tools are installed: `xcode-select --install`
4. Verify Homebrew is working: `brew doctor`
