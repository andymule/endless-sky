@echo off
REM Build (Release) and run Catch2 unit tests
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-windows.ps1" -Config Release -Tests
if errorlevel 1 exit /b 1
