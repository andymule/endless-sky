@echo off
setlocal enabledelayedexpansion

REM Set MSYS2 root path
set MSYS2_ROOT=C:\msys64

echo ========================================
echo Installing MSYS2 Dependencies for Dynamix
echo ========================================

REM Check if MSYS2 is installed
if not exist "%MSYS2_ROOT%\msys2_shell.cmd" (
    echo ERROR: MSYS2 not found at %MSYS2_ROOT%
    echo Please install MSYS2 from https://www.msys2.org/
    echo After installation, run this script again.
    pause
    exit /b 1
)

echo MSYS2 found at %MSYS2_ROOT%
echo.
echo This will install the required packages for building Dynamix:
echo - mingw-w64-x86_64-cmake
echo - mingw-w64-x86_64-ninja  
echo - mingw-w64-x86_64-clang
echo - mingw-w64-x86_64-SDL2
echo - mingw-w64-x86_64-glew
echo - mingw-w64-x86_64-openal
echo - mingw-w64-x86_64-libpng
echo - mingw-w64-x86_64-libjpeg-turbo
echo - mingw-w64-x86_64-zlib
echo.

set /p CONFIRM="Continue with installation? (y/N): "
if /i not "%CONFIRM%"=="y" (
    echo Installation cancelled.
    pause
    exit /b 0
)

echo.
echo Installing packages...
echo.

REM Install the required packages
"%MSYS2_ROOT%\msys2_shell.cmd" -mingw64 -defterm -here -no-start -c "pacman -S --noconfirm mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-clang mingw-w64-x86_64-SDL2 mingw-w64-x86_64-glew mingw-w64-x86_64-openal mingw-w64-x86_64-libpng mingw-w64-x86_64-libjpeg-turbo mingw-w64-x86_64-zlib"

if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to install packages!
    echo Please try running the command manually in MSYS2 MinGW64 terminal:
    echo pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-clang mingw-w64-x86_64-SDL2 mingw-w64-x86_64-glew mingw-w64-x86_64-openal mingw-w64-x86_64-libpng mingw-w64-x86_64-libjpeg-turbo mingw-w64-x86_64-zlib
    pause
    exit /b 1
)

echo.
echo ========================================
echo Installation completed successfully!
echo ========================================
echo.
echo You can now run:
echo - build-windows-fast.bat (for release build)
echo - build-windows-debug.bat (for debug build)
echo.
pause 