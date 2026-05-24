# Kenshi Network Launcher — Инструкция по установке

## Шаг 1: Установка Visual Studio 2022 Build Tools

### Способ A: Автоматическая установка (рекомендуется)

1. Откройте **PowerShell от имени администратора**
2. Скопируйте и выполните эту команду:

```powershell
Set-ExecutionPolicy Bypass -Scope Process -Force
irm https://raw.githubusercontent.com/KavaMan1337/KenshiNetworkLauncher/main/scripts/install-toolchain.ps1 | iex
```

Или скачайте скрипт вручную:
- Файл: `scripts/install-toolchain.ps1` из [репозитория](https://github.com/KavaMan1337/KenshiNetworkLauncher/tree/main/scripts)
- Правой кнопкой → "Выполнить с PowerShell"

### Способ B: Ручная установка

1. Скачайте Visual Studio 2022 Build Tools:
   https://aka.ms/vs/17/release/vs_buildtools.exe

2. Запустите установщик от имени администратора и выберите:
   - **Разработка классических приложений на C++**
   - (остальное можно оставить по умолчанию)

3. Установите CMake 3.20+ если его нет:
   https://cmake.org/download/ (скачайте `cmake-*-windows-x86_64.msi`)

---

## Шаг 2: Сборка проекта

Откройте **PowerShell** (обычный, не от администратора) в папке проекта:

```powershell
cd "C:\Users\argus\Desktop\KenshiNetworkLauncher"
.\build.bat
```

Или вручную:

```powershell
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
cmake --build build --config Release
```

**Результат:**
- `build\HostLauncher\Release\KenshiLauncher.Host.exe`
- `build\ClientLauncher\Release\KenshiLauncher.Client.exe`

---

## Шаг 3: Запуск

1. Скопируйте оба .exe файла в удобное место
2. Запускайте когда нужно играть

---

## Быстрая проверка (без сборки)

Если хотите просто скачать готовые .exe из GitHub Releases:
https://github.com/KavaMan1337/KenshiNetworkLauncher/releases
