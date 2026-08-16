@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

set MSYS2_BIN=C:\msys64\mingw64\bin
set REQUIRED_DLLS=SDL2.dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll

echo Copying runtime DLLs from MSYS2...

for %%D in (build build_debug static_build) do (
    if not exist "%%D" mkdir "%%D"
    for %%F in (%REQUIRED_DLLS%) do (
        if exist "%MSYS2_BIN%\%%F" (
            copy /Y "%MSYS2_BIN%\%%F" "%%D\" >nul
            echo   %%D\%%F
        ) else if exist "libs\dlls\%%F" (
            copy /Y "libs\dlls\%%F" "%%D\" >nul
            echo   %%D\%%F (from libs\dlls)
        ) else (
            echo   WARNING: %%F not found
        )
    )
    if exist "assets" xcopy "assets" "%%D\assets\" /E /I /Y >nul
)

echo Done.
