# Kenshi Network Launcher — Installer Script
# Требует запуск от имени администратора!
# Этот скрипт устанавливает:
#   1. CMake 3.30 (если не установлен)
#   2. Visual Studio 2022 Build Tools с C++ Desktop workload

Write-Host "=== Kenshi Network Launcher — Установка инструментов ===" -ForegroundColor Cyan

# Проверка админских прав
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "[ERROR] Запустите PowerShell от имени администратора!" -ForegroundColor Red
    Write-Host "ПКМ на Пуск -> Windows PowerShell (Администратор)" -ForegroundColor Yellow
    exit 1
}

$ErrorActionPreference = "Continue"

# 1. CMake
Write-Host "`n[1/2] Установка CMake 3.30..." -ForegroundColor Yellow
$cmakeUrl = "https://github.com/Kitware/CMake/releases/download/v3.30.0/cmake-3.30.0-windows-x86_64.msi"
$cmakeTemp = "$env:TEMP\cmake-installer.msi"

if (-not (Get-Command cmake -ErrorAction SilentlyContinue) -and -not (Test-Path "C:\Program Files\CMake\bin\cmake.exe")) {
    Write-Host "Скачивание CMake..." -ForegroundColor Gray
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    try {
        Invoke-WebRequest -Uri $cmakeUrl -OutFile $cmakeTemp -UseBasicParsing -TimeoutSec 120
        Write-Host "Установка CMake..." -ForegroundColor Gray
        Start-Process msiexec -ArgumentList "/i `"$cmakeTemp`" /quiet ADD_CMAKE_TO_PATH=System" -Wait
        Remove-Item $cmakeTemp -Force -ErrorAction SilentlyContinue
        Write-Host "CMake установлен." -ForegroundColor Green
    } catch {
        Write-Host "CMake: пропускаем (или уже установлен)" -ForegroundColor Gray
    }
} else {
    Write-Host "CMake уже установлен." -ForegroundColor Green
}

# 2. VS Build Tools
Write-Host "`n[2/2] Установка Visual Studio 2022 Build Tools..." -ForegroundColor Yellow
Write-Host "Скачивание (~3 GB)..." -ForegroundColor Gray

$vsUrl = "https://aka.ms/vs/17/release/vs_buildtools.exe"
$vsTemp = "$env:TEMP\vs_buildtools.exe"

try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $vsUrl -OutFile $vsTemp -UseBasicParsing -TimeoutSec 300
} catch {
    Write-Host "[ERROR] Не удалось скачать VS Build Tools. Проверьте интернет." -ForegroundColor Red
    Write-Host "Скачайте вручную: https://aka.ms/vs/17/release/vs_buildtools.exe" -ForegroundColor Yellow
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

Write-Host "Установка Visual Studio (может занять 10-30 минут)..." -ForegroundColor Gray
$proc = Start-Process $vsTemp -ArgumentList $vsArgs -PassThru
$proc.WaitForExit()
$exitCode = $proc.ExitCode

Remove-Item $vsTemp -Force -ErrorAction SilentlyContinue

if ($exitCode -eq 0 -or $exitCode -eq 3010) {
    Write-Host ""
    Write-Host "============================================" -ForegroundColor Green
    Write-Host "[OK] Visual Studio Build Tools установлены!" -ForegroundColor Green
    Write-Host "============================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Далее откройте НОВУЮ PowerShell (не эту) и выполните:" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "    cd `"C:\Users\argus\Desktop\KenshiNetworkLauncher`"" -ForegroundColor White
    Write-Host "    cmake -G `"Visual Studio 17 2022`" -A x64 -S . -B build" -ForegroundColor White
    Write-Host "    cmake --build build --config Release" -ForegroundColor White
    Write-Host ""
    Write-Host "Готовые .exe будут в:" -ForegroundColor Gray
    Write-Host "    build\HostLauncher\Release\KenshiLauncher.Host.exe" -ForegroundColor Gray
    Write-Host "    build\ClientLauncher\Release\KenshiLauncher.Client.exe" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Или просто запустите build.bat в папке проекта." -ForegroundColor Gray
} else {
    Write-Host "[WARNING] Установка завершена с кодом: $exitCode" -ForegroundColor Yellow
    Write-Host "Попробуйте установить вручную: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Yellow
}