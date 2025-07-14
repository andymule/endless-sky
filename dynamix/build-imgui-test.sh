#!/bin/bash

echo "Building ImGui Test Application..."
echo "=================================="

# Create build directory
if [ ! -d build_imgui_test ]; then
    mkdir build_imgui_test
fi

# Clean CMake cache and files
rm -rf build_imgui_test/CMakeCache.txt build_imgui_test/CMakeFiles

# Temporarily rename CMakeLists_imgui_test.txt to CMakeLists.txt
if [ -f CMakeLists.txt ]; then
    mv CMakeLists.txt CMakeLists_backup.txt
fi
mv CMakeLists_imgui_test.txt CMakeLists.txt

cd build_imgui_test

# Configure with CMake using MSYS2/MinGW64
echo "Configuring with CMake..."
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_MAKE_PROGRAM=ninja -S .. -B .

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed"
    exit 1
fi

# Restore CMakeLists.txt in root
cd ..
mv CMakeLists.txt CMakeLists_imgui_test.txt
if [ -f CMakeLists_backup.txt ]; then
    mv CMakeLists_backup.txt CMakeLists.txt
fi
cd build_imgui_test

# Build the project
echo "Building project..."
cmake --build . --config Release

if [ $? -ne 0 ]; then
    echo "Error: Build failed"
    exit 1
fi

echo ""
echo "Build completed successfully!"
echo "Executable: build_imgui_test/imgui_test.exe"
echo ""
echo "To run the application:"
echo "1. Navigate to build_imgui_test directory"
echo "2. Run: ./imgui_test.exe"
echo "" 