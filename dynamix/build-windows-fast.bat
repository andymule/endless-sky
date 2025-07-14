@echo off
setlocal enabledelayedexpansion

REM Set paths
set MSYS2_ROOT=C:\msys64
set MINGW_BIN=%MSYS2_ROOT%\mingw64\bin
set BUILD_DIR=%~dp0build
set SRC_DIR=%~dp0

echo ========================================
echo Building Dynamix on Windows with MSYS2
echo ========================================

REM Check if MSYS2 is installed
if not exist "%MSYS2_ROOT%\msys2_shell.cmd" (
    echo ERROR: MSYS2 not found at %MSYS2_ROOT%
    echo Please install MSYS2 from https://www.msys2.org/
    pause
    exit /b 1
)

REM Check if we need to force a clean rebuild (when optimizations change)
set FORCE_REBUILD=0
if not exist "%BUILD_DIR%\CMakeCache.txt" set FORCE_REBUILD=1

REM Force rebuild if CMakeLists.txt is newer than CMakeCache.txt
if exist "%BUILD_DIR%\CMakeCache.txt" (
    for /f "tokens=1,2 delims= " %%a in ('dir /tc "%SRC_DIR%CMakeLists.txt" ^| findstr "CMakeLists.txt"') do set CMKLIST_TIME=%%a %%b
    for /f "tokens=1,2 delims= " %%a in ('dir /tc "%BUILD_DIR%\CMakeCache.txt" ^| findstr "CMakeCache.txt"') do set CACHE_TIME=%%a %%b
    if "!CMKLIST_TIME!" gtr "!CACHE_TIME!" set FORCE_REBUILD=1
)

echo.
echo Step 1: Building with MSYS2/MinGW64...
echo.

REM Build using MSYS2 MinGW64 shell
if %FORCE_REBUILD%==1 (
    echo CMakeLists.txt changed, forcing clean rebuild...
    "%MSYS2_ROOT%\msys2_shell.cmd" -mingw64 -defterm -here -no-start -c "cd /c/Users/xxsha/source/endless-sky/dynamix && rm -rf build && mkdir build && cd build && cmake -G 'Ninja' -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_MAKE_PROGRAM=ninja -DENABLE_UNITY_BUILD=ON -DENABLE_PCH=OFF -DENABLE_CCACHE=ON -DENABLE_LTO=OFF .."
) else (
    echo CMake already configured, skipping configuration...
)

if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Step 2: Compiling with Ninja (maximal concurrency)...
echo.

REM Run the build with maximal concurrency
"%MSYS2_ROOT%\msys2_shell.cmd" -mingw64 -defterm -here -no-start -c "cd /c/Users/xxsha/source/endless-sky/dynamix/build && ninja -j 0"

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo Step 3: Copying required DLLs...
echo.

REM Create a list of required DLLs and their locations
set REQUIRED_DLLS=SDL2.dll;libgcc_s_seh-1.dll;libstdc++-6.dll;libwinpthread-1.dll;libpng16-16.dll;libjpeg-8.dll;libopenal-1.dll;libzlib1.dll

REM Copy each required DLL
for %%d in (%REQUIRED_DLLS%) do (
    echo Copying %%d...
    
    REM Try to find the DLL in various locations
    set DLL_FOUND=0
    
    REM Check MSYS2 bin directory first
    if exist "%MINGW_BIN%\%%d" (
        copy "%MINGW_BIN%\%%d" "%BUILD_DIR%\" >nul 2>&1
        if !ERRORLEVEL! equ 0 (
            echo   - Copied from %MINGW_BIN%
            set DLL_FOUND=1
        )
    )
    
    REM Check if DLL is already in build directory
    if exist "%BUILD_DIR%\%%d" (
        if !DLL_FOUND! equ 0 (
            echo   - Already present in build directory
            set DLL_FOUND=1
        )
    )
    
    REM Check Windows System32 as fallback
    if !DLL_FOUND! equ 0 (
        if exist "C:\Windows\System32\%%d" (
            copy "C:\Windows\System32\%%d" "%BUILD_DIR%\" >nul 2>&1
            if !ERRORLEVEL! equ 0 (
                echo   - Copied from System32
                set DLL_FOUND=1
            )
        )
    )
    
    REM Check Windows SysWOW64 as fallback
    if !DLL_FOUND! equ 0 (
        if exist "C:\Windows\SysWOW64\%%d" (
            copy "C:\Windows\SysWOW64\%%d" "%BUILD_DIR%\" >nul 2>&1
            if !ERRORLEVEL! equ 0 (
                echo   - Copied from SysWOW64
                set DLL_FOUND=1
            )
        )
    )
    
    REM Warn if DLL not found
    if !DLL_FOUND! equ 0 (
        echo   - WARNING: %%d not found in any expected location
    )
)

echo.
echo Step 4: Copying assets...
echo.

REM Copy assets directory if it exists
if exist "%SRC_DIR%assets" (
    echo Copying assets directory...
    xcopy "%SRC_DIR%assets" "%BUILD_DIR%\assets\" /E /I /Y >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        echo   - Assets copied successfully
    ) else (
        echo   - WARNING: Failed to copy assets
    )
) else (
    echo   - No assets directory found, skipping
)

echo.
echo Step 5: Verifying build...
echo.

REM Check if executable exists
if exist "%BUILD_DIR%\dynamix.exe" (
    echo ✅ Build successful! Executable created: %BUILD_DIR%\dynamix.exe
    echo.
    echo File size: 
    for %%A in ("%BUILD_DIR%\dynamix.exe") do echo   %%~zA bytes
    echo.
    
    REM List all files in build directory
    echo Build directory contents:
    dir "%BUILD_DIR%\*.exe" "%BUILD_DIR%\*.dll" 2>nul | findstr /v "Directory"
    
    echo.
    echo Step 6: Testing executable...
    echo.
    
    REM Test if executable runs (non-blocking)
    echo Starting dynamix.exe for testing...
    start "" "%BUILD_DIR%\dynamix.exe"
    
    echo.
    echo ✅ Build and test complete!
    echo.
    echo The application should now be running. If it doesn't start,
    echo check that all required DLLs are present in the build directory.
    echo.
    echo Build directory: %BUILD_DIR%
    echo.
    
) else (
    echo ❌ ERROR: dynamix.exe not found after build!
    echo.
    echo Build directory contents:
    dir "%BUILD_DIR%\"
    pause
    exit /b 1
)

echo ========================================
echo Build process completed successfully!
echo ========================================
echo.
echo Next time you run this script, it will:
echo - Skip CMake configuration (unless CMakeLists.txt changed)
echo - Only rebuild changed files (incremental build)
echo - Use maximal concurrency (all CPU cores)
echo - Copy all required DLLs automatically
echo.
pause 