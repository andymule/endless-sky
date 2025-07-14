@echo off
setlocal

set BUILD_DIR=%~dp0build
set DEBUG_BUILD_DIR=%~dp0build_debug

REM Print all environment variables for debugging
set

echo ========================================
echo Dynamix Launcher
========================================

echo BUILD_DIR = %BUILD_DIR%
echo DEBUG_BUILD_DIR = %DEBUG_BUILD_DIR%

if exist "%BUILD_DIR%\dynamix.exe" (
    echo Found release build: %BUILD_DIR%\dynamix.exe
    cd /d "%BUILD_DIR%"
    start "" "dynamix.exe"
    echo Launched release build.
) else (
    if exist "%DEBUG_BUILD_DIR%\dynamix.exe" (
        echo Found debug build: %DEBUG_BUILD_DIR%\dynamix.exe
        cd /d "%DEBUG_BUILD_DIR%"
        start "" "dynamix.exe"
        echo Launched debug build.
    ) else (
        echo ERROR: No dynamix.exe found!
        exit /b 1
    )
)

echo Done. 