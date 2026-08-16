@echo off
setlocal
cd /d "%~dp0"

set BUILD_DIR=%~dp0build
set DEBUG_BUILD_DIR=%~dp0build_debug

if exist "%BUILD_DIR%\dynamix.exe" (
    echo Launching %BUILD_DIR%\dynamix.exe
    start "" /D "%BUILD_DIR%" "dynamix.exe"
    exit /b 0
)

if exist "%DEBUG_BUILD_DIR%\dynamix.exe" (
    echo Launching %DEBUG_BUILD_DIR%\dynamix.exe
    start "" /D "%DEBUG_BUILD_DIR%" "dynamix.exe"
    exit /b 0
)

echo ERROR: dynamix.exe not found. Build first with build-windows-fast.bat
exit /b 1
