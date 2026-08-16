# How to Build and Run Dynamix on Windows

Windows builds use **MSYS2 MinGW64 + Clang + Ninja**. Do not mix MSVC and MinGW on the same build directory.

## Quick start

1. Install [MSYS2](https://www.msys2.org/) to `C:\msys64`
2. Double-click `install-msys2-deps.bat` (cmake, ninja, clang, pkgconf, SDL2, gdb)
3. Double-click `build-windows-fast.bat` (release) or `build-windows-debug.bat` (debug)
4. Double-click `run-dynamix.bat`, or run tests with `run-tests.bat`

From PowerShell in this folder:

```powershell
.\build-windows.ps1 -Config Release
.\build-windows.ps1 -Config Debug
.\build-windows.ps1 -Config Release -Tests
.\build-windows.ps1 -Config Release -Clean -Tests
.\run-dynamix.bat
```

VS Code / Cursor: default build task is **Build Dynamix (Release - Windows)**. Use **Build and Test (Windows)** to compile Catch2 tests and run `[unit]`. Debug uses MinGW `gdb` (`C:\msys64\mingw64\bin\gdb.exe`) via the C/C++ extension.

## What you need

| Package | Purpose |
| --- | --- |
| `mingw-w64-x86_64-cmake` | Configure |
| `mingw-w64-x86_64-ninja` | Build |
| `mingw-w64-x86_64-clang` | Compiler |
| `mingw-w64-x86_64-pkgconf` | Find SDL2 |
| `mingw-w64-x86_64-SDL2` | Windowing / audio backend |
| `mingw-w64-x86_64-gdb` | Debugging from the IDE |

ImGui, SoLoud, Signalsmith Stretch, and nlohmann/json are fetched by CMake, as is Catch2 when tests are enabled; versions are pinned near the top of `CMakeLists.txt`. PNG, JPEG, OpenAL, GLEW, and minizip are **not** required (legacy Endless Sky leftovers).

## Layout

- Release binary: `build/dynamix.exe`
- Debug binary: `build_debug/dynamix.exe`
- Tests: `build/dynamix_tests.exe` (built when `-Tests` / `-DBUILD_TESTS=ON`)
- Runtime DLLs are copied next to the exe from `C:\msys64\mingw64\bin`

## Tests

```powershell
.\run-tests.bat
# or, after a tests-enabled build:
.\build\dynamix_tests.exe [unit]
.\build\dynamix_tests.exe [audio]
.\build\dynamix_tests.exe
```

`run-tests.bat` builds with tests enabled and runs the `[unit]` tier. The full run adds the audio, integration, and roundtrip tiers, which mix real audio through SoLoud's null driver — no display and no audio device needed anywhere in the suite.

## Troubleshooting

- **MSYS2 not found**: install to `C:\msys64`, or edit `$MSYS2_ROOT` in `build-windows.ps1`
- **cmake/ninja/clang missing**: run `install-msys2-deps.bat`
- **Linker says `cannot open output file dynamix.exe: Permission denied`**: something still has the binary open. `build-windows.ps1` closes copies it started, so this usually means an instance launched another way; close it and rebuild
- **SDL2.dll missing at runtime**: run `copy-dlls.bat`, or rebuild (the script copies DLLs)
- **IDE debug will not start**: install `mingw-w64-x86_64-gdb` and the VS Code **C/C++** extension (`ms-vscode.cpptools`)
- **Do not** configure the same `build/` folder with MSVC after using MinGW
