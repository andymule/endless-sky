@echo off
echo Running dynamix.exe in MSYS2 environment...
C:\msys64\msys2_shell.cmd -mingw64 -defterm -here -no-start -c "cd /c/Users/xxsha/source/endless-sky/dynamix/build && ./dynamix.exe"
pause 