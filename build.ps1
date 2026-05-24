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

# Check CMake
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    $cmakePath = "C:\Program Files\CMake\bin\cmake.exe"
    if (-not (Test-Path $cmakePath)) {
        Write-Host "[ERROR] CMake not found." -ForegroundColor Red
        Write-Host ""
        Write-Host "Install CMake 3.30:" -ForegroundColor Yellow
        Write-Host "https://github.com/Kitware/CMake/releases/download/v3.30.0/cmake-3.30.0-windows-x86_64.msi" -ForegroundColor Gray
        Write-Host ""
        Write-Host "Or install VS 2022 Build Tools (includes CMake):" -ForegroundColor Yellow
        Write-Host "https://aka.ms/vs/17/release/vs_buildtools.exe" -ForegroundColor Gray
        Write-Host ""
        Write-Host "Then run this script again: .\build.ps1" -ForegroundColor Cyan
        Read-Host "Press Enter to exit"
        exit 1
    }
}
Write-Host "[OK] CMake found" -ForegroundColor Green

# Find VS Developer Command Prompt
$vsvars = $null
$vsPaths = @(
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
)
foreach ($p in $vsPaths) {
    if (Test-Path $p) { $vsvars = $p; break }
}
if ($vsvars) {
    Write-Host "[OK] Visual Studio found: $vsvars" -ForegroundColor Green
} else {
    Write-Host "[WARNING] vcvars64.bat not found. If build fails, install VS 2022 Build Tools." -ForegroundColor Yellow
}

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
