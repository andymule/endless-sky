# SoLoud SDL2 Integration Test

This is a simple test program that uses the SoLoud audio engine with SDL2 backend to play sounds. It's designed to be compatible with Endless Sky which uses SDL2 for graphics and input handling.

## Dependencies

- CMake 3.10 or higher
- C++11 compatible compiler
- SDL2 development libraries

## About SoLoud with SDL2

SoLoud provides a simple audio API that can use SDL2 as its backend. This is ideal for integration with Endless Sky since:

1. Endless Sky already uses SDL2 for graphics and input 
2. SoLoud's SDL2 backend is robust and well-supported
3. The integration can be done incrementally without disrupting the existing audio code

The test demonstrates playing a simple alarm sound using the SDL2 static backend.

## Building

```bash
# Run the build script (recommended)
./build.sh
```

Or manually:

```bash
# Create a build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build .
```

## Platform-Specific Setup

### macOS
Install SDL2 via Homebrew:
```bash
brew install sdl2
```

### Linux
Install SDL2 development packages:
```bash
# Ubuntu/Debian
sudo apt-get install libsdl2-dev

# Fedora
sudo dnf install SDL2-devel

# Arch Linux
sudo pacman -S sdl2
```

### Windows
- Install SDL2 development libraries
- Set up the appropriate paths in your build environment
- Or use vcpkg to manage SDL2 dependencies

## Running

After building, run the executable:

```bash
# From the build directory
./soloud_test
```

The program will play the alarm.wav sound file using SoLoud's SDL2 static backend.

## Integration with Endless Sky

See `integration.md` for details on how to integrate SoLoud with the main Endless Sky project.

## Troubleshooting

- If you encounter SDL2 initialization errors, make sure SDL2 is properly installed for your platform
- If sound doesn't play, check that the alarm.wav file is in the correct location
- For linking errors, ensure your SDL2 libraries are correctly detected by CMake 