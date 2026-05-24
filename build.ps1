# Kenshi Network Launcher — Сборка проекта (PowerShell)
# Запуск: .\build.ps1

$ErrorActionPreference = "Continue"
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $projectRoot) { $projectRoot = "C:\Users\argus\Desktop\KenshiNetworkLauncher" }
$buildDir = Join-Path $projectRoot "build"

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Kenshi Network Launcher — Сборка проекта" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Проверка CMake
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    $cmakePath = "C:\Program Files\CMake\bin\cmake.exe"
    if (-not (Test-Path $cmakePath)) {
        Write-Host "[ERROR] CMake не найден." -ForegroundColor Red
        Write-Host ""
        Write-Host "Установите CMake: https://github.com/Kitware/CMake/releases/download/v3.30.0/cmake-3.30.0-windows-x86_64.msi" -ForegroundColor Yellow
        Write-Host "Или запустите: scripts\install-toolchain.ps1 (от администратора)" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Или установите Visual Studio 2022 Build Tools с C++ workload." -ForegroundColor Yellow
        Write-Host "Скачать: https://aka.ms/vs/17/release/vs_buildtools.exe" -ForegroundColor Yellow
        Read-Host "Нажмите Enter для выхода"
        exit 1
    }
}
Write-Host "[OK] CMake найден" -ForegroundColor Green

# VS Developer Command Prompt
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
    Write-Host "[OK] Visual Studio найден: $vsvars" -ForegroundColor Green
    Write-Host "Настройка переменных окружения..." -ForegroundColor Gray
    cmd /c " `"$vsvars" > nul 2>&1 && set | findstr /I cmake" | Out-Null
} else {
    Write-Host "[WARNING] vcvars64.bat не найден. Если cmake не найдет компилятор - установите VS." -ForegroundColor Yellow
}

# Очистка старой сборки
if (Test-Path $buildDir) {
    Write-Host "[...] Очистка старой сборки..." -ForegroundColor Gray
    Remove-Item $buildDir -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host ""
Write-Host "[1/2] Генерация проекта (CMake)..." -ForegroundColor Yellow
$genResult = cmake -G "Visual Studio 17 2022" -A x64 -S $projectRoot -B $buildDir 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] CMake генерация не удалась." -ForegroundColor Red
    Write-Host $genResult -ForegroundColor Red
    Read-Host "Нажмите Enter для выхода"
    exit 1
}
Write-Host "[OK] Проект сгенерирован" -ForegroundColor Green

Write-Host ""
Write-Host "[2/2] Сборка (Release)..." -ForegroundColor Yellow
$buildResult = cmake --build $buildDir --config Release 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Сборка не удалась." -ForegroundColor Red
    Write-Host $buildResult -ForegroundColor Red
    Read-Host "Нажмите Enter для выхода"
    exit 1
}
Write-Host "[OK] Сборка завершена" -ForegroundColor Green

Write-Host ""
Write-Host "============================================" -ForegroundColor Green
Write-Host "  Сборка завершена успешно!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host ""
Write-Host "Готовые файлы:" -ForegroundColor White
$hostExe = Join-Path $buildDir "HostLauncher\Release\KenshiLauncher.Host.exe"
$clientExe = Join-Path $buildDir "ClientLauncher\Release\KenshiLauncher.Client.exe"
Write-Host "  Host:   $hostExe" -ForegroundColor Gray
Write-Host "  Client: $clientExe" -ForegroundColor Gray
Write-Host ""
Read-Host "Нажмите Enter для выхода"