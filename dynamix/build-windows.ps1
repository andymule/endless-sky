# Unified Windows build for Dynamix (MSYS2/MinGW64 + Clang + Ninja)
param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [switch]$Tests,
    [switch]$Clean,
    [switch]$Run
)

$ErrorActionPreference = "Stop"

$MSYS2_ROOT = "C:\msys64"
$SrcDir = $PSScriptRoot
$BuildDirName = if ($Config -eq "Debug") { "build_debug" } else { "build" }
$BuildDir = Join-Path $SrcDir $BuildDirName
$MingwBin = Join-Path $MSYS2_ROOT "mingw64\bin"

function ConvertTo-MsysPath([string]$WinPath) {
    $full = [System.IO.Path]::GetFullPath($WinPath) -replace '\\', '/'
    if ($full -match '^([A-Za-z]):') {
        return '/' + $Matches[1].ToLower() + $full.Substring(2)
    }
    return $full
}

function Stop-BuildOutputProcesses([string]$Dir) {
    foreach ($name in @("dynamix", "dynamix_tests")) {
        foreach ($proc in @(Get-Process -Name $name -ErrorAction SilentlyContinue)) {
            $path = try { $proc.Path } catch { $null }
            if (-not $path -or -not $path.StartsWith($Dir, [System.StringComparison]::OrdinalIgnoreCase)) {
                continue
            }
            Write-Host "Closing running $name (PID $($proc.Id)) so the linker can overwrite it" -ForegroundColor Yellow
            try { $proc.CloseMainWindow() | Out-Null } catch { }
            if (-not $proc.WaitForExit(3000)) {
                Stop-Process -Id $proc.Id -Force
                $proc.WaitForExit(3000) | Out-Null
            }
        }
    }
}

function Invoke-Msys([string]$Command) {
    $shell = Join-Path $MSYS2_ROOT "msys2_shell.cmd"
    Write-Host "  $Command" -ForegroundColor DarkGray
    & $shell -mingw64 -defterm -here -no-start -c $Command
    if ($LASTEXITCODE -ne 0) {
        throw "MSYS2 command failed ($LASTEXITCODE): $Command"
    }
}

Write-Host "========================================" -ForegroundColor Green
Write-Host "Dynamix Windows build ($Config)" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

if (-not (Test-Path (Join-Path $MSYS2_ROOT "msys2_shell.cmd"))) {
    Write-Host "ERROR: MSYS2 not found at $MSYS2_ROOT" -ForegroundColor Red
    Write-Host "Install from https://www.msys2.org/ then run install-msys2-deps.bat" -ForegroundColor Red
    exit 1
}

foreach ($tool in @("cmake", "ninja", "clang", "pkg-config")) {
    $path = Join-Path $MingwBin "$tool.exe"
    if (-not (Test-Path $path)) {
        Write-Host "ERROR: $tool.exe not found in $MingwBin" -ForegroundColor Red
        Write-Host "Run install-msys2-deps.bat" -ForegroundColor Red
        exit 1
    }
}

$needConfigure = $Clean -or -not (Test-Path (Join-Path $BuildDir "CMakeCache.txt"))
if (-not $needConfigure -and (Test-Path (Join-Path $BuildDir "CMakeCache.txt"))) {
    $listsTime = (Get-Item (Join-Path $SrcDir "CMakeLists.txt")).LastWriteTime
    $cacheTime = (Get-Item (Join-Path $BuildDir "CMakeCache.txt")).LastWriteTime
    if ($listsTime -gt $cacheTime) { $needConfigure = $true }
}

$unity = if ($Config -eq "Release") { "ON" } else { "OFF" }
$testsFlag = if ($Tests) { "ON" } else { "OFF" }
if ($Tests -and -not (Test-Path (Join-Path $BuildDir "dynamix_tests.exe"))) {
    $needConfigure = $true
}

Push-Location $SrcDir
try {
$msysBuild = ConvertTo-MsysPath $BuildDir
$msysSrc = ConvertTo-MsysPath $SrcDir

    if ($needConfigure) {
        Write-Host "Configuring CMake ($Config, tests=$testsFlag)..." -ForegroundColor Yellow
        if ($Clean -and (Test-Path $BuildDir)) {
            Remove-Item $BuildDir -Recurse -Force
        }
        New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
        Invoke-Msys "cd '$msysBuild' && cmake -G Ninja -DCMAKE_BUILD_TYPE=$Config -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_MAKE_PROGRAM=ninja -DENABLE_UNITY_BUILD=$unity -DENABLE_PCH=OFF -DENABLE_CCACHE=ON -DENABLE_LTO=OFF -DUSE_PREDOWNLOADED_DEPS=OFF -DBUILD_TESTS=$testsFlag '$msysSrc'"
    } else {
        Write-Host "CMake already configured, building incrementally..." -ForegroundColor Yellow
    }

    Stop-BuildOutputProcesses $BuildDir

    Write-Host "Compiling..." -ForegroundColor Yellow
    $ninjaTarget = if ($Tests) { "dynamix dynamix_tests" } else { "dynamix" }
    Invoke-Msys "cd '$msysBuild' && ninja -j 0 $ninjaTarget"
} finally {
    Pop-Location
}

$dlls = @("SDL2.dll", "libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")
Write-Host "Copying runtime DLLs..." -ForegroundColor Yellow
foreach ($dll in $dlls) {
    $src = Join-Path $MingwBin $dll
    if (Test-Path $src) {
        Copy-Item $src (Join-Path $BuildDir $dll) -Force
        Write-Host "  $dll" -ForegroundColor Green
    } else {
        Write-Host "  WARNING: $dll not in $MingwBin" -ForegroundColor Yellow
    }
}

$assetsSrc = Join-Path $SrcDir "assets"
if (Test-Path $assetsSrc) {
    Copy-Item $assetsSrc (Join-Path $BuildDir "assets") -Recurse -Force
}

$exe = Join-Path $BuildDir "dynamix.exe"
if (-not (Test-Path $exe)) {
    Write-Host "ERROR: $exe was not produced" -ForegroundColor Red
    exit 1
}

Write-Host "Build OK: $exe" -ForegroundColor Green

if ($Tests) {
    $testExe = Join-Path $BuildDir "dynamix_tests.exe"
    if (-not (Test-Path $testExe)) {
        Write-Host "ERROR: tests were requested but $testExe is missing" -ForegroundColor Red
        exit 1
    }
    Write-Host "Running unit tests..." -ForegroundColor Yellow
    Push-Location $BuildDir
    try {
        & $testExe "[unit]"
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Unit tests failed ($LASTEXITCODE)" -ForegroundColor Red
            exit $LASTEXITCODE
        }
        Write-Host "Unit tests passed. Audio/integration tests: .\build\dynamix_tests.exe" -ForegroundColor Green
    } finally {
        Pop-Location
    }
}

if ($Run) {
    Start-Process $exe -WorkingDirectory $BuildDir
}

Write-Host "Done." -ForegroundColor Green
