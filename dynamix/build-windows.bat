@echo off
setlocal

REM Set paths
set MSYS2_ROOT=C:\msys64
set MINGW_BIN=%MSYS2_ROOT%\mingw64\bin
set BUILD_DIR=%~dp0build
set SRC_DIR=%~dp0

REM Build using MSYS2 MinGW64 shell
"%MSYS2_ROOT%\msys2_shell.cmd" -mingw64 -defterm -here -no-start -c "cd /c/Users/xxsha/source/endless-sky/dynamix && rm -rf build && mkdir build && cd build && cmake -G 'Ninja' -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ .. && ninja"

REM Copy required DLLs
copy "%MINGW_BIN%\libgcc_s_seh-1.dll" "%BUILD_DIR%" >nul
copy "%MINGW_BIN%\libwinpthread-1.dll" "%BUILD_DIR%" >nul
copy "%MINGW_BIN%\libstdc++-6.dll" "%BUILD_DIR%" >nul
copy "%MINGW_BIN%\SDL2.dll" "%BUILD_DIR%" >nul

REM Optionally run the executable (uncomment if desired)
REM pushd "%BUILD_DIR%"
REM dynamix.exe
REM popd

endlocal 