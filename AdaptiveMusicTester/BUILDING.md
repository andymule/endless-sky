# Building the Adaptive Music Tester

This document provides detailed instructions for building the Adaptive Music Tester on various platforms.

## Prerequisites

- A C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10 or higher
- SDL2 and SDL2_mixer development libraries
- Git (to clone Dear ImGui)

## Step 1: Clone Dear ImGui

The project requires Dear ImGui. If you haven't already, clone it into the project directory:

```bash
git clone https://github.com/ocornut/imgui.git AdaptiveMusicTester/imgui
```

## Step 2: Install SDL2 and SDL2_mixer

### macOS

Using Homebrew:

```bash
brew install sdl2 sdl2_mixer
```

### Linux (Ubuntu/Debian)

```bash
sudo apt install libsdl2-dev libsdl2-mixer-dev
```

### Windows

#### Using vcpkg:

```bash
vcpkg install sdl2 sdl2-mixer
```

#### Using pre-built binaries:

1. Download SDL2 development libraries from [SDL website](https://www.libsdl.org/download-2.0.php)
2. Download SDL2_mixer development libraries from [SDL_mixer website](https://www.libsdl.org/projects/SDL_mixer/)
3. Extract both to a convenient location
4. When configuring CMake, set the SDL2_DIR and SDL2_MIXER_DIR variables to point to these locations

## Step 3: Configure and Build

### Using CMake from the command line

```bash
cd AdaptiveMusicTester
mkdir build
cd build
cmake ..
```

Then, depending on your platform:

#### macOS/Linux

```bash
make
```

#### Windows (with Visual Studio)

```bash
cmake --build . --config Release
```

Or open the generated solution file in Visual Studio and build from there.

### Using an IDE with CMake support

Many IDEs like CLion, Visual Studio, or VSCode with CMake extensions can open and build CMake projects directly.

1. Open the AdaptiveMusicTester folder in your IDE
2. Configure the CMake project
3. Build the project using your IDE's build command

## Step 4: Run the application

After building, you can run the application from the build directory:

```bash
# From the build directory
./adaptive_music_tester   # On macOS/Linux
.\Release\adaptive_music_tester.exe  # On Windows
```

## Troubleshooting

### SDL2 or SDL2_mixer not found

If CMake can't find SDL2 or SDL2_mixer, you may need to specify their locations:

```bash
cmake .. -DSDL2_DIR=/path/to/sdl2 -DSDL2_MIXER_DIR=/path/to/sdl2_mixer
```

### Include path issues

If you encounter include path errors, you may need to update the include paths in the source files to match your system's setup. Common variations include:

- `#include <SDL.h>` vs `#include <SDL2/SDL.h>`
- `#include <SDL_mixer.h>` vs `#include <SDL2/SDL_mixer.h>`

Adjust these as needed based on your platform and installation method.

### Compilation errors with Dear ImGui

Make sure you've cloned the Dear ImGui repository correctly and that all required files are present. The CMakeLists.txt expects to find them in the `imgui` directory at the root of the project. 