@echo off
setlocal enabledelayedexpansion

REM Set paths
set DLL_SOURCE_DIR=%~dp0libs\dlls

REM Ensure we're in the correct directory
cd /d "%~dp0"

echo ========================================
echo Copying Required DLLs for Dynamix from libs/dlls

echo ========================================

REM Only copy the core runtime DLLs as per how-to-windows.md
set REQUIRED_DLLS=SDL2.dll;libgcc_s_seh-1.dll;libstdc++-6.dll;libwinpthread-1.dll

REM Copy DLLs to all build directories
call :copy_dlls_to_dir build
call :copy_dlls_to_dir build_debug
call :copy_dlls_to_dir static_build

echo ========================================
echo DLL copying completed for all build directories!
echo ========================================
echo.
echo Build directories:
echo - Release: build
echo - Debug: build_debug
echo - Static: static_build
echo.

goto :eof

REM Function to copy DLLs to a specific directory
:copy_dlls_to_dir
set TARGET_DIR=%~1

echo.
echo Copying DLLs to: %TARGET_DIR%
echo.

REM Create target directory if it doesn't exist
if not exist "%TARGET_DIR%" (
    echo Creating directory: %TARGET_DIR%
    mkdir "%TARGET_DIR%"
)

REM Copy each required DLL from libs/dlls
for %%d in (%REQUIRED_DLLS%) do (
    echo Copying %%d...
    if exist "%DLL_SOURCE_DIR%\%%d" (
        copy /Y "%DLL_SOURCE_DIR%\%%d" "%TARGET_DIR%\" >nul 2>&1
        if !ERRORLEVEL! equ 0 (
            echo   - Copied from libs/dlls
        )
    ) else (
        echo   - WARNING: %%d not found in libs/dlls
    )
)

REM Copy assets directory if it exists
if exist "%~dp0assets" (
    echo.
    echo Copying assets directory...
    xcopy "%~dp0assets" "%TARGET_DIR%\assets\" /E /I /Y >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        echo   - Assets copied successfully
    ) else (
        echo   - WARNING: Failed to copy assets
    )
) else (
    echo   - No assets directory found, skipping
)

echo.
echo DLL copying complete for: %TARGET_DIR%
echo.
goto :eof 