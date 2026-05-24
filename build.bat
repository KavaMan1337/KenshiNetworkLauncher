@echo off
chcp 65001 >nul 2>&1
title Kenshi Network Launcher — Сборка

echo ============================================
echo   Kenshi Network Launcher — Сборка проекта
echo ============================================
echo.

REM Проверка CMake
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake не найден. Установите CMake или VS Build Tools.
    echo Запустите scripts\install-toolchain.ps1 от имени администратора
    pause
    exit /b 1
)

REM Определяем путь к VS Developer Command Prompt
if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    echo [OK] VS Build Tools найдены
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    echo [OK] VS Build Tools найдены
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    echo [OK] VS Community найден
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    echo [OK] VS Professional найден
) else (
    echo [WARNING] vcvars64.bat не найден. CMake может не найти компилятор.
    echo Убедитесь что Visual Studio 2022 с C++ workload установлена.
)

echo.
echo [1/2] Генерация проекта (CMake)...
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake генерация не удалась.
    pause
    exit /b 1
)

echo.
echo [2/2] Сборка (Release)...
cmake --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Сборка не удалась.
    pause
    exit /b 1
)

echo.
echo ============================================
echo   Сборка завершена!
echo ============================================
echo.
echo Готовые файлы:
echo.
echo   build\HostLauncher\Release\KenshiLauncher.Host.exe
echo   build\ClientLauncher\Release\KenshiLauncher.Client.exe
echo.
echo Скопируйте оба .exe в удобное место.
echo.
pause
