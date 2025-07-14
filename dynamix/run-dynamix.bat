@echo off
setlocal

REM Set paths
set BUILD_DIR=%~dp0build
set DEBUG_BUILD_DIR=%~dp0build_debug

echo ========================================
echo Dynamix Launcher
echo ========================================

REM Check if release build exists
if exist "%BUILD_DIR%\dynamix.exe" (
    echo Starting Dynamix (Release Build)...
    echo Build directory: %BUILD_DIR%
    echo.
    cd /d "%BUILD_DIR%"
    start "" "dynamix.exe"
    echo ✅ Dynamix started successfully!
) else if exist "%DEBUG_BUILD_DIR%\dynamix.exe" (
    echo Starting Dynamix (Debug Build)...
    echo Build directory: %DEBUG_BUILD_DIR%
    echo.
    cd /d "%DEBUG_BUILD_DIR%"
    start "" "dynamix.exe"
    echo ✅ Dynamix started successfully!
) else (
    echo ❌ ERROR: No dynamix.exe found!
    echo.
    echo Please run one of the build scripts first:
    echo - build-windows-fast.bat (for release build)
    echo - build-windows-debug.bat (for debug build)
    echo.
    pause
    exit /b 1
)

echo.
echo The application should now be running.
echo Press any key to exit this launcher...
pause >nul 