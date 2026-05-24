# Kenshi Network Launcher

Удобный лаунчер для хостинга и подключения к мультиплеерной игре Kenshi (Steam, версия 1.0.68+) через **Radmin VPN**.

[Скачать Visual Studio 2022 Build Tools](https://aka.ms/vs/17/release/vs_buildtools.exe) · [Скачать CMake](https://github.com/Kitware/CMake/releases/download/v3.30.0/cmake-3.30.0-windows-x86_64.msi) · [Скачать Radmin VPN](https://www.radmin-vpn.com/download/)

---

## Быстрый старт

### Установка инструментов (один раз)

Скачайте и запустите (от имени администратора PowerShell):

| Инструмент | Ссылка | Размер |
|-----------|--------|--------|
| **Visual Studio 2022 Build Tools** | [vs_buildtools.exe](https://aka.ms/vs/17/release/vs_buildtools.exe) | ~3 ГБ |
| **CMake 3.30** | [cmake-3.30.0.msi](https://github.com/Kitware/CMake/releases/download/v3.30.0/cmake-3.30.0-windows-x86_64.msi) | ~30 МБ |

Или автоматически (PowerShell от администратора):
```powershell
cd "C:\Users\argus\Desktop\KenshiNetworkLauncher\scripts"
.\install-toolchain.ps1
```

### Сборка проекта

```powershell
cd "C:\Users\argus\Desktop\KenshiNetworkLauncher"
.\build.ps1
```

Готовые .exe появятся в:
- `build\HostLauncher\Release\KenshiLauncher.Host.exe`
- `build\ClientLauncher\Release\KenshiLauncher.Client.exe`

---

## Что это

Два .exe файла:
- **KenshiLauncher.Host.exe** — лаунчер хоста. Запускает выделенный сервер Kenshi и показывает ваш IP для друзей.
- **KenshiLauncher.Client.exe** — лаунчер клиента. Подключается к серверу и запускает игру.

Внутри используется проект [Kenshi-Online](https://github.com/The404Studios/Kenshi-Online) — open-source мультиплеерный мод для Kenshi.

---

## Как играть

### Шаг 1 — Настройка Radmin VPN

1. Скачайте [Radmin VPN](https://www.radmin-vpn.com/) и установите
2. Создайте сеть (или присоединитесь к существующей)
3. Добавьте всех игроков в одну сеть

### Шаг 2 — Хост (создатель мира)

1. Запустите `KenshiLauncher.Host.exe`
2. Нажмите **"Установить мод"** — лаунчер автоматически скачает Kenshi-Online с GitHub
3. Настройте сервер: имя, порт (default 27800), макс. игроков (до 5), PvP
4. Нажмите **"Запустить сервер"**
5. Скопируйте IP:Port (кнопка "Копировать IP") и отправьте друзьям

### Шаг 3 — Клиенты (остальные игроки)

1. Запустите `KenshiLauncher.Client.exe`
2. Нажмите **"Установить мод"**
3. Введите своё имя игрока
4. Введите IP:Port от хоста
5. Нажмите **"Играть"**
6. В главном меню Kenshi нажмите **MULTIPLAYER** или кнопку **`** (Ё) для внутриигрового меню подключения

### Управление в игре

| Клавиша | Действие |
|---------|---------|
| **`** (Ё/тильда) | Открыть меню мультиплеера |
| F1 | Переключить HUD |
| Enter | Чат |
| Tab | Список игроков |

---

## Требования

| Компонент | Версия |
|-----------|--------|
| ОС | Windows 10/11 (x64) |
| Игра | Kenshi 1.0.68+ (Steam) |
| Сеть | Radmin VPN (подсети 100.x.x.x / 10.x.x.x) |
| Макс. игроков | 5 (настраивается) |
| Порт по умолчанию | 27800 UDP |

---

## Структура проекта

```
scripts/
  install-toolchain.ps1   — автоустановка VS + CMake
build.ps1                 — сборка проекта (запуск через .\build.ps1)

src/
  LauncherCommon/         — общий код
    SteamPath.cpp         — поиск Kenshi через Steam Registry
    GitHubReleases.cpp    — скачивание Kenshi-Online с GitHub
    ModDeployer.cpp       — установка модов, патч Plugins_x64.cfg
    NetworkUtils.cpp       — определение IP (Radmin VPN)
  HostLauncher/           — KenshiLauncher.Host.exe
  ClientLauncher/         — KenshiLauncher.Client.exe
```

---

## Сборка вручную

```powershell
# Клонировать
git clone https://github.com/KavaMan1337/KenshiNetworkLauncher.git
cd KenshiNetworkLauncher

# Сборка (через скрипт)
.\build.ps1

# Или вручную через cmake
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
cmake --build build --config Release
```

---

## Лицензия

MIT License — свободное использование и модификация.
Kenshi-Online лицензирован отдельно (MIT).
