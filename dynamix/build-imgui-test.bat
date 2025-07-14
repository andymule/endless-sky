@echo off
echo Building ImGui Test Application for Windows (MSYS2/MinGW64)
echo =========================================================

REM Check if we're in the right directory
if not exist "test_imgui_simple.cpp" (
    echo Error: test_imgui_simple.cpp not found in current directory
    echo Please run this script from the project root directory
    pause
    exit /b 1
)

REM Launch MSYS2 MinGW64 shell and run the build script
echo Launching MSYS2 MinGW64 shell...
C:\msys64\msys2_shell.cmd -mingw64 -defterm -here -no-start -c "./build-imgui-test.sh"

if %ERRORLEVEL% neq 0 (
    echo Error: Build failed
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable: build_imgui_test\imgui_test.exe
echo.
echo To run the application:
echo 1. Navigate to build_imgui_test directory
echo 2. Run: imgui_test.exe
echo.
pause 