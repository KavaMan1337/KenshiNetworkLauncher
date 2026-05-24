# Kenshi Network Launcher - Toolchain Installer
# Run as Administrator!

Write-Host "=== Kenshi Network Launcher - Toolchain Installer ===" -ForegroundColor Cyan

# Admin check
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "[ERROR] Run PowerShell as Administrator!" -ForegroundColor Red
    Write-Host "Right-click Start -> Windows PowerShell (Admin)" -ForegroundColor Yellow
    exit 1
}

$ErrorActionPreference = "Continue"

# 1. CMake
Write-Host "`n[1/2] Installing CMake 3.30..." -ForegroundColor Yellow
$cmakeUrl = "https://github.com/Kitware/CMake/releases/download/v3.30.0/cmake-3.30.0-windows-x86_64.msi"
$cmakeTemp = "$env:TEMP\cmake-installer.msi"

$cmakeInstalled = (Get-Command cmake -ErrorAction SilentlyContinue) -or (Test-Path "C:\Program Files\CMake\bin\cmake.exe")
if (-not $cmakeInstalled) {
    Write-Host "Downloading CMake..." -ForegroundColor Gray
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    try {
        Invoke-WebRequest -Uri $cmakeUrl -OutFile $cmakeTemp -UseBasicParsing -TimeoutSec 120
        Write-Host "Installing CMake (window may appear)..." -ForegroundColor Gray
        Start-Process msiexec -ArgumentList "/i `"$cmakeTemp`" /quiet ADD_CMAKE_TO_PATH=System" -Wait
        Remove-Item $cmakeTemp -Force -ErrorAction SilentlyContinue
        Write-Host "CMake installed." -ForegroundColor Green
    } catch {
        Write-Host "CMake: skipped (or already installed)" -ForegroundColor Gray
    }
} else {
    Write-Host "CMake already installed." -ForegroundColor Green
}

# 2. VS Build Tools
Write-Host "`n[2/2] Installing Visual Studio 2022 Build Tools..." -ForegroundColor Yellow
Write-Host "Downloading (~3 GB)..." -ForegroundColor Gray

$vsUrl = "https://aka.ms/vs/17/release/vs_buildtools.exe"
$vsTemp = "$env:TEMP\vs_buildtools.exe"

try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $vsUrl -OutFile $vsTemp -UseBasicParsing -TimeoutSec 300
} catch {
    Write-Host "[ERROR] Failed to download VS Build Tools. Check internet." -ForegroundColor Red
    Write-Host "Download manually: https://aka.ms/vs/17/release/vs_buildtools.exe" -ForegroundColor Yellow
    exit 1
}

$vsArgs = @(
    "--quiet", "--wait", "--norestart", "--nocache",
    "--add", "Microsoft.VisualStudio.Workload.VCTools",
    "--add", "Microsoft.VisualStudio.Component.VC.CMake",
    "--add", "Microsoft.VisualStudio.Component.VC.CLC",
    "--add", "Microsoft.VisualStudio.Component.VC.Runtimes.x64.x86",
    "--includeRecommended"
)

Write-Host "Installing Visual Studio (may take 10-30 minutes)..." -ForegroundColor Gray
$proc = Start-Process $vsTemp -ArgumentList $vsArgs -PassThru
$proc.WaitForExit()
$exitCode = $proc.ExitCode

Remove-Item $vsTemp -Force -ErrorAction SilentlyContinue

if ($exitCode -eq 0 -or $exitCode -eq 3010) {
    Write-Host ""
    Write-Host "============================================" -ForegroundColor Green
    Write-Host "[OK] Visual Studio Build Tools installed!" -ForegroundColor Green
    Write-Host "============================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Open a NEW PowerShell (not this one) and run:" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "    cd `"C:\Users\argus\Desktop\KenshiNetworkLauncher`"" -ForegroundColor White
    Write-Host "    .\build.ps1" -ForegroundColor White
    Write-Host ""
} else {
    Write-Host "[WARNING] Exit code: $exitCode" -ForegroundColor Yellow
    Write-Host "Try installing manually: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Yellow
}
