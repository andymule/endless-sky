@echo off
REM Debug build (wrapper around build-windows.ps1)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-windows.ps1" -Config Debug %*
if errorlevel 1 exit /b 1
