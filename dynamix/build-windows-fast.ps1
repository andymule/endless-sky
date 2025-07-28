# PowerShell build script for Dynamix on Windows with MSYS2

Write-Host "========================================" -ForegroundColor Green
Write-Host "Building Dynamix on Windows with MSYS2" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

# Set paths
$MSYS2_ROOT = "C:\msys64"
$BUILD_DIR = Join-Path $PSScriptRoot "build"
$SRC_DIR = $PSScriptRoot

# Check if MSYS2 is installed
if (-not (Test-Path "$MSYS2_ROOT\msys2_shell.cmd")) {
    Write-Host "ERROR: MSYS2 not found at $MSYS2_ROOT" -ForegroundColor Red
    Write-Host "Please install MSYS2 from https://www.msys2.org/" -ForegroundColor Red
    Read-Host "Press Enter to continue"
    exit 1
}

# Check if required MSYS2 packages are installed
Write-Host "Checking MSYS2 dependencies..." -ForegroundColor Yellow

$cmakeResult = & "$MSYS2_ROOT\msys2_shell.cmd" -mingw64 -defterm -here -no-start -c "which cmake"
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: cmake not found in MSYS2/MinGW64" -ForegroundColor Red
    Write-Host "Please install required packages by running in MSYS2 MinGW64 terminal:" -ForegroundColor Red
    Write-Host "pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-clang" -ForegroundColor Red
    Read-Host "Press Enter to continue"
    exit 1
}

$ninjaResult = & "$MSYS2_ROOT\msys2_shell.cmd" -mingw64 -defterm -here -no-start -c "which ninja"
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: ninja not found in MSYS2/MinGW64" -ForegroundColor Red
    Write-Host "Please install required packages by running in MSYS2 MinGW64 terminal:" -ForegroundColor Red
    Write-Host "pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-clang" -ForegroundColor Red
    Read-Host "Press Enter to continue"
    exit 1
}

$clangResult = & "$MSYS2_ROOT\msys2_shell.cmd" -mingw64 -defterm -here -no-start -c "which clang"
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: clang not found in MSYS2/MinGW64" -ForegroundColor Red
    Write-Host "Please install required packages by running in MSYS2 MinGW64 terminal:" -ForegroundColor Red
    Write-Host "pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-clang" -ForegroundColor Red
    Read-Host "Press Enter to continue"
    exit 1
}

Write-Host "All MSYS2 dependencies found!" -ForegroundColor Green

# Check if we need to force a clean rebuild
$FORCE_REBUILD = $false
if (-not (Test-Path "$BUILD_DIR\CMakeCache.txt")) {
    $FORCE_REBUILD = $true
}

# Force rebuild if CMakeLists.txt is newer than CMakeCache.txt
if (Test-Path "$BUILD_DIR\CMakeCache.txt") {
    $cmakeListsTime = (Get-Item "$SRC_DIR\CMakeLists.txt").LastWriteTime
    $cacheTime = (Get-Item "$BUILD_DIR\CMakeCache.txt").LastWriteTime
    if ($cmakeListsTime -gt $cacheTime) {
        $FORCE_REBUILD = $true
    }
}

Write-Host ""
Write-Host "Step 1: Building with MSYS2/MinGW64..." -ForegroundColor Yellow
Write-Host ""

# Build using MSYS2 MinGW64 shell
if ($FORCE_REBUILD) {
    Write-Host "CMakeLists.txt changed, forcing clean rebuild..." -ForegroundColor Yellow
    $cmakeResult = & "$MSYS2_ROOT\msys2_shell.cmd" -mingw64 -defterm -here -no-start -c "rm -rf build && mkdir build && cd build && cmake -G 'Ninja' -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_MAKE_PROGRAM=ninja -DENABLE_UNITY_BUILD=ON -DENABLE_PCH=OFF -DENABLE_CCACHE=ON -DENABLE_LTO=OFF -DUSE_PREDOWNLOADED_DEPS=OFF .."
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: CMake configuration failed!" -ForegroundColor Red
        Read-Host "Press Enter to continue"
        exit 1
    }
} else {
    Write-Host "CMake already configured, skipping configuration..." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Step 2: Compiling with Ninja (maximal concurrency)..." -ForegroundColor Yellow
Write-Host ""

# Run the build with maximal concurrency
$buildResult = & "$MSYS2_ROOT\msys2_shell.cmd" -mingw64 -defterm -here -no-start -c "cd build && ninja -j 0"
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed!" -ForegroundColor Red
    Read-Host "Press Enter to continue"
    exit 1
}

Write-Host ""
Write-Host "Step 3: Copying required DLLs..." -ForegroundColor Yellow
Write-Host ""

# Copy required DLLs from MSYS2
$requiredDlls = @("SDL2.dll", "libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")

foreach ($dll in $requiredDlls) {
    Write-Host "Copying $dll..." -ForegroundColor Cyan
    $sourcePath = "$MSYS2_ROOT\mingw64\bin\$dll"
    $destPath = "$BUILD_DIR\$dll"
    
    if (Test-Path $sourcePath) {
        Copy-Item $sourcePath $destPath -Force
        Write-Host "  - Copied from MSYS2" -ForegroundColor Green
    } else {
        Write-Host "  - WARNING: $dll not found in MSYS2" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "Step 4: Copying assets..." -ForegroundColor Yellow
Write-Host ""

# Copy assets directory if it exists
if (Test-Path "$SRC_DIR\assets") {
    Write-Host "Copying assets directory..." -ForegroundColor Cyan
    $assetsDest = "$BUILD_DIR\assets"
    if (Test-Path $assetsDest) {
        Remove-Item $assetsDest -Recurse -Force
    }
    Copy-Item "$SRC_DIR\assets" $assetsDest -Recurse -Force
    Write-Host "  - Assets copied successfully" -ForegroundColor Green
} else {
    Write-Host "  - No assets directory found, skipping" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "You can now run the application with:" -ForegroundColor Cyan
Write-Host "cd build && .\dynamix.exe" -ForegroundColor White
Write-Host "" 