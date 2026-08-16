@echo off
setlocal
set MSYS2_ROOT=C:\msys64

echo ========================================
echo Installing MSYS2 packages for Dynamix
echo ========================================

if not exist "%MSYS2_ROOT%\msys2_shell.cmd" (
    echo ERROR: MSYS2 not found at %MSYS2_ROOT%
    echo Install from https://www.msys2.org/ then re-run this script.
    exit /b 1
)

echo Packages: cmake ninja clang pkgconf SDL2 gdb
echo.

"%MSYS2_ROOT%\msys2_shell.cmd" -mingw64 -defterm -here -no-start -c "pacman -S --needed --noconfirm mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-clang mingw-w64-x86_64-pkgconf mingw-w64-x86_64-SDL2 mingw-w64-x86_64-gdb"

if errorlevel 1 (
    echo ERROR: pacman failed. Open an MSYS2 MinGW64 terminal and run:
    echo   pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-clang mingw-w64-x86_64-pkgconf mingw-w64-x86_64-SDL2 mingw-w64-x86_64-gdb
    exit /b 1
)

echo.
echo Done. Next:
echo   build-windows-fast.bat
echo   run-tests.bat
echo   run-dynamix.bat
