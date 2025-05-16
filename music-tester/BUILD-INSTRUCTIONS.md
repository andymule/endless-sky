# Building Music Tester

This document provides step-by-step instructions to build the Music Tester application.

## Requirements

- CMake 3.19 or higher
- C++20 compatible compiler
- vcpkg (for managing dependencies)
- OAML (Open Adaptive Music Library) installed on your system

## Dependencies

The Music Tester requires the following libraries:
- SDL2
- OpenGL & GLEW
- OpenAL
- minizip
- OAML (system-installed)
- Dear ImGui (fetched automatically)

## Installing OAML

### On macOS

The easiest way is to install OAML from source:

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

## Build Instructions

### Using the Build Script (Recommended)

On macOS:
```bash
./music-tester/build-macos.sh
```

This script will:
1. Check for required system libraries
2. Install minizip using vcpkg
3. Configure and build the music-tester application

If you need to clean and rebuild:
```bash
./music-tester/build-macos.sh clean
```

### Manual Build with vcpkg

If you encounter issues with minizip, follow these special instructions:

1. Make sure vcpkg is initialized:
   ```bash
   git submodule update --init --recursive
   ```

2. Build with explicit minizip path:
   ```bash
   mkdir -p build
   cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
     -DES_BUILD_MUSIC_TESTER=ON \
     -Dunofficial-minizip_DIR=$(pwd)/vcpkg/packages/minizip_$(uname -m)-$(uname | tr '[:upper:]' '[:lower:]')/share/unofficial-minizip
   ```

   On macOS, you can use:
   ```bash
   cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
     -DES_BUILD_MUSIC_TESTER=ON \
     -Dunofficial-minizip_DIR=$(pwd)/vcpkg/packages/minizip_arm64-osx/share/unofficial-minizip
   ```

3. Build the music-tester target:
   ```bash
   cmake --build build --target music-tester -j$(nproc)
   ```
   
   On macOS, you can use:
   ```bash
   cmake --build build --target music-tester -j$(sysctl -n hw.ncpu)
   ```

### Without vcpkg

If you have all the dependencies installed system-wide:

1. Configure:
   ```bash
   mkdir -p build
   cmake -B build -DES_USE_VCPKG=OFF -DES_BUILD_MUSIC_TESTER=ON
   ```

2. Build:
   ```bash
   cmake --build build --target music-tester
   ```

## Running the Application

After building, the executable will be located in the build directory. Run it with:

```bash
./build/music-tester [optional_music_directory]
```

## Troubleshooting

If you encounter issues with missing libraries:

1. Verify OAML is installed correctly:
   ```bash
   ls -la /usr/local/lib/liboaml*
   ```

2. For runtime errors about missing libraries, you may need to set the library path:
   ```bash
   export DYLD_LIBRARY_PATH=/usr/local/lib:$DYLD_LIBRARY_PATH
   ```

3. For build-time errors related to finding OAML, make sure pkg-config can find it:
   ```bash
   pkg-config --cflags --libs oaml
   ```