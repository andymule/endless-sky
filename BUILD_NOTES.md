# Endless Sky Build Notes

## Quick Start Commands

### Working Build Commands (macOS ARM64)
```bash
# Clean build from scratch
rm -rf build && mkdir build

# Configure with vcpkg dependencies
cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
  -Dunofficial-minizip_DIR=$(pwd)/vcpkg/packages/minizip_arm64-osx/share/unofficial-minizip \
  -DLIBMAD_INCLUDE_DIR=$(pwd)/vcpkg/packages/libmad_arm64-osx/include \
  -DLIBMAD_LIB_RELEASE=$(pwd)/vcpkg/packages/libmad_arm64-osx/lib/libmad.a \
  -DLIBMAD_LIB_DEBUG=$(pwd)/vcpkg/packages/libmad_arm64-osx/debug/lib/libmad.a

# Build with all cores
cmake --build build -j$(sysctl -n hw.ncpu)
```

### VSCode Build Tasks
- **Cmd+Shift+B**: Default build (Full Build Debug)
- **Cmd+Shift+P → "Tasks: Run Task"**: Access all build tasks
  - `Full Build Debug` - Clean + configure + build debug
  - `Full Build Release` - Clean + configure + build release  
  - `Quick Build Debug` - Configure + build debug (no clean)
  - `Quick Build Release` - Configure + build release (no clean)

### VSCode Launch Configurations
- **F5** or **Run → Start Debugging**: Launch with debugger
- Available configs in `.vscode/launch.json`:
  - `Launch Endless Sky (Debug)` - Debug build of main game
  - `Launch Endless Sky (Release)` - Release build of main game
  - `Launch music-tester (Debug)` - Debug build of music tester
  - `Launch music-tester (Release)` - Release build of music tester
  - `Attach to Endless Sky` - Attach debugger to running game
  - `Attach to music-tester` - Attach debugger to running music tester

## Build Targets

### Main Targets
- **EndlessSky**: Main game executable → `build/endless-sky`
- **music-tester**: Adaptive music testing tool → `build/music-tester/music-tester`
- **EndlessSkyTests**: Unit tests → `build/tests/EndlessSkyTests`

### Running Built Executables
```bash
# Run main game
./build/endless-sky

# Run music tester (needs sound_staging directory)
./build/music-tester/music-tester

# Run tests
./build/tests/EndlessSkyTests
```

## Critical Build Fixes Applied

### 1. OpenGL Framework Fix (CMakeLists.txt lines 179-188)
**Problem**: macOS trying to link against X11 GL libraries instead of native framework
**Solution**: Use native macOS OpenGL framework
```cmake
if(APPLE)
    # On macOS, explicitly use the native framework to avoid X11 GL libraries
    find_library(OPENGL_FRAMEWORK OpenGL)
    target_link_libraries(ExternalLibraries INTERFACE ${OPENGL_FRAMEWORK})
    # Apple deprecated OpenGL in MacOS 10.14, but we don't care.
    target_compile_definitions(EndlessSkyLib PUBLIC GL_SILENCE_DEPRECATION)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(ExternalLibraries INTERFACE OpenGL::GL)
    # GLEW is only needed on Linux and Windows.
    target_link_libraries(ExternalLibraries INTERFACE GLEW::glew)
endif()
```

### 2. Minizip Dependency Fix
**Problem**: CMake finds minizip package but compilation can't find headers
**Solution**: Two-part fix:
1. **CMake command**: Explicit vcpkg package paths:
   - `-Dunofficial-minizip_DIR=$(pwd)/vcpkg/packages/minizip_arm64-osx/share/unofficial-minizip`
   - Plus LIBMAD explicit paths for completeness
2. **CMakeLists.txt fix**: Added explicit include directory detection (lines 95-105 and 158-160):
   ```cmake
   # Explicitly add minizip include directory for macOS ARM64
   if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
       if(DEFINED unofficial-minizip_DIR)
           # Calculate path: share/unofficial-minizip -> minizip_arm64-osx -> include
           get_filename_component(MINIZIP_VCPKG_DIR "${unofficial-minizip_DIR}" DIRECTORY)
           get_filename_component(MINIZIP_VCPKG_DIR "${MINIZIP_VCPKG_DIR}" DIRECTORY)
           set(MINIZIP_INCLUDE_DIR "${MINIZIP_VCPKG_DIR}/include")
           if(EXISTS "${MINIZIP_INCLUDE_DIR}/minizip/unzip.h")
               message(STATUS "Found minizip headers at: ${MINIZIP_INCLUDE_DIR}")
           endif()
       endif()
   endif()
   
   # Add minizip include directory if explicitly set
   if(DEFINED MINIZIP_INCLUDE_DIR)
       target_include_directories(ExternalLibraries INTERFACE "${MINIZIP_INCLUDE_DIR}")
   endif()
   ```

## Dependencies

### vcpkg Packages (ARM64 macOS)
- minizip (unofficial-minizip)
- libmad  
- SDL2, OpenAL, PNG, JPEG, ZLIB
- OpenGL (system framework)

### Development Tools
- CMake 3.16+
- Xcode Command Line Tools
- vcpkg package manager
- VSCode with CodeLLDB extension

## Project Structure

```
endless-sky/
├── source/           # Main game source code
├── music-tester/     # Adaptive music testing tool
├── tests/            # Unit tests
├── vcpkg/            # Package manager and dependencies
├── build/            # Build output directory
├── .vscode/          # VSCode configuration
│   ├── tasks.json    # Build tasks
│   ├── launch.json   # Debug configurations
│   └── settings.json # C++ development settings
└── BUILD_NOTES.md    # This file
```

## Troubleshooting

### Common Issues
1. **"minizip/unzip.h not found"**: Use complete cmake command with all explicit paths
2. **OpenGL linking errors**: Ensure OpenGL framework fix is in CMakeLists.txt
3. **VSCode task not found**: Check launch.json references correct task names
4. **music-tester fails**: Needs sound_staging directory (auto-copied during build)

### Clean Rebuild
If build gets corrupted:
```bash
rm -rf build
# Then run full cmake configure + build commands above
```

### Git Branch
Currently on: `soloud-w-SDL2` branch with adaptive music library integration

## Performance Notes
- Uses Ninja when available for faster builds
- Parallel compilation with `-j$(sysctl -n hw.ncpu)` (8 cores on this machine)
- Incremental builds work after initial setup
- VSCode tasks handle generator conflicts automatically 