@echo off
echo Running ImGui Test Application...
echo =================================

if not exist "build_imgui_test\imgui_test.exe" (
    echo Error: imgui_test.exe not found. Please run build-imgui-test.bat first.
    pause
    exit /b 1
)

cd build_imgui_test
imgui_test.exe

echo.
echo Application finished.
pause 