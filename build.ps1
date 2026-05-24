# Kenshi Network Launcher - Build Script
# Run: .\build.ps1

$ErrorActionPreference = "Continue"
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $projectRoot) { $projectRoot = "C:\Users\argus\Desktop\KenshiNetworkLauncher" }
$buildDir = Join-Path $projectRoot "build"

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Kenshi Network Launcher - Build Script" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Find MSVC toolchain
$msvcRoot = $null
$msvcVers = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.30.30705",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.40.31712",
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC"
)
foreach ($v in $msvcVers) {
    if (Test-Path "$v\bin\Hostx64\x64\cl.exe") { $msvcRoot = $v; break }
}

if ($msvcRoot) {
    Write-Host "[OK] MSVC compiler found: $msvcRoot" -ForegroundColor Green
    $msvcBin = "$msvcRoot\bin\Hostx64\x64"
    $msvcLib = "$msvcRoot\lib\x64"
    $vcRedist = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\x64"
    $sdkBin = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64"
    $sdkInc = "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt"
    $sdkIncWinRT = "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt"

    $env:PATH = "$msvcBin;$sdkBin;$env:PATH"
    $env:LIB = "$msvcLib;$vcRedist;$env:LIB"
    $env:INCLUDE = "$sdkInc;$sdkIncWinRT;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.30.30705\include;$env:INCLUDE"
    $env:VCToolsInstallDir = "$msvcRoot\..\.."
    Write-Host "[OK] Environment configured" -ForegroundColor Green
} else {
    Write-Host "[WARNING] MSVC compiler not found. Build will likely fail." -ForegroundColor Yellow
    Write-Host "Install VS 2022 Community with C++ workload" -ForegroundColor Yellow
}

# Check CMake
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    $cmakePath = "C:\Program Files\CMake\bin\cmake.exe"
    if (-not (Test-Path $cmakePath)) {
        Write-Host "[ERROR] CMake not found." -ForegroundColor Red
        Write-Host "Install: https://github.com/Kitware/CMake/releases/download/v3.30.0/cmake-3.30.0-windows-x86_64.msi" -ForegroundColor Gray
        Write-Host "Or: https://aka.ms/vs/17/release/vs_buildtools.exe" -ForegroundColor Gray
        Read-Host "Press Enter to exit"
        exit 1
    }
}
Write-Host "[OK] CMake found: $(cmake --version 2>&1 | Select-Object -First 1)" -ForegroundColor Green

# Clean old build
if (Test-Path $buildDir) {
    Write-Host "[...] Cleaning old build..." -ForegroundColor Gray
    Remove-Item $buildDir -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host ""
Write-Host "[1/2] Generating project (CMake)..." -ForegroundColor Yellow
$genResult = cmake -G "Visual Studio 17 2022" -A x64 -S $projectRoot -B $buildDir 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] CMake generation failed." -ForegroundColor Red
    Write-Host $genResult -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}
Write-Host "[OK] Project generated" -ForegroundColor Green

Write-Host ""
Write-Host "[2/2] Building (Release)..." -ForegroundColor Yellow
$buildResult = cmake --build $buildDir --config Release 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Build failed." -ForegroundColor Red
    Write-Host $buildResult -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}
Write-Host "[OK] Build complete" -ForegroundColor Green

Write-Host ""
Write-Host "============================================" -ForegroundColor Green
Write-Host "  Build successful!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host ""
Write-Host "Output files:" -ForegroundColor White
$hostExe = Join-Path $buildDir "HostLauncher\Release\KenshiLauncher.Host.exe"
$clientExe = Join-Path $buildDir "ClientLauncher\Release\KenshiLauncher.Client.exe"
Write-Host "  Host:   $hostExe" -ForegroundColor Gray
Write-Host "  Client: $clientExe" -ForegroundColor Gray
Write-Host ""
Read-Host "Press Enter to exit"