# ImGui Test Application for Dynamix

This is a simple ImGui test application that demonstrates the basic setup for Windows using MSYS2/MinGW64, following the guidelines in `how-to-windows.md`.

## Features

- **SDL2 Integration**: Uses SDL2 for window management and input handling
- **OpenGL 3.3**: Modern OpenGL rendering with GL3W loader
- **ImGui Interface**: Complete ImGui demo with multiple windows
- **Windows Compatibility**: Follows MSYS2/MinGW64 setup guidelines
- **Proper Entry Point**: Uses `#define SDL_MAIN_HANDLED` to avoid infinite loops

## Files

- `test_imgui_simple.cpp` - Main application source code
- `CMakeLists_imgui_test.txt` - CMake configuration for the test
- `build-imgui-test.bat` - Windows build script
- `README_imgui_test.md` - This file

## Prerequisites

1. **MSYS2/MinGW64 Environment**: Make sure you have MSYS2 installed with MinGW64 toolchain
2. **Required Packages**: Install the following packages in MSYS2:
   ```bash
   pacman -S mingw-w64-x86_64-cmake
   pacman -S mingw-w64-x86_64-ninja
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-sdl2
   pacman -S mingw-w64-x86_64-opengl-headers
   ```

## Building

### Option 1: Using the Batch Script (Recommended)

1. Open MSYS2 MinGW64 terminal
2. Navigate to the project directory
3. Run the build script:
   ```bash
   ./build-imgui-test.bat
   ```

### Option 2: Manual Build

1. Open MSYS2 MinGW64 terminal
2. Navigate to the project directory
3. Create build directory:
   ```bash
   mkdir build_imgui_test
   cd build_imgui_test
   ```
4. Configure with CMake:
   ```bash
   cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_MAKE_PROGRAM=ninja ../CMakeLists_imgui_test.txt
   ```
5. Build the project:
   ```bash
   cmake --build . --config Release
   ```

## Running

1. Navigate to the build directory:
   ```bash
   cd build_imgui_test
   ```
2. Run the application:
   ```bash
   ./imgui_test.exe
   ```

## What You'll See

The application will display:
- **Main Window**: "Hello, Dynamix!" with various ImGui widgets
- **Demo Window**: Full ImGui demo with all available widgets
- **Another Window**: Simple additional window (toggleable)

Features demonstrated:
- Sliders, buttons, checkboxes
- Color picker
- Real-time FPS counter
- Multiple windows
- Dark theme
- Keyboard navigation

## Key Implementation Details

### Entry Point Setup
```cpp
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>

int main(int argc, char* argv[]) {
    // Standard C++ main function
}
```

### OpenGL Context
```cpp
SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
```

### ImGui Integration
```cpp
ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
ImGui_ImplOpenGL3_Init("#version 330");
```

## Troubleshooting

### Infinite Loop Issues
If the application starts and immediately restarts or enters an infinite loop:
1. Make sure you're using `#define SDL_MAIN_HANDLED`
2. Use a standard `main()` function, not `WinMain()`
3. Don't include any WinMain forwarder files

### Missing DLLs
If you get "missing DLL" errors:
1. Make sure you're running from the MSYS2 MinGW64 environment
2. Check that SDL2.dll is in the same directory as the executable
3. Verify that all required MSYS2 packages are installed

### Build Errors
If CMake or build fails:
1. Ensure you're using the MSYS2 MinGW64 terminal
2. Verify all required packages are installed
3. Check that the CMakeLists.txt file path is correct

## Next Steps

This test application provides a solid foundation for:
- Adding more complex ImGui interfaces
- Integrating with the main Dynamix application
- Testing audio features with SoLoud
- Adding file browser functionality
- Implementing custom themes and styling

The setup follows all the Windows guidelines from `how-to-windows.md` and should work reliably in the MSYS2/MinGW64 environment. 