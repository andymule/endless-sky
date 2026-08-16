@echo off
REM Release build (wrapper around build-windows.ps1)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-windows.ps1" -Config Release %*
if errorlevel 1 exit /b 1
