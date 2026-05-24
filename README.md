# Kenshi Network Launcher

Удобный лаунчер для хостинга и подключения к мультиплеерной игре Kenshi (Steam, версия 1.0.68+) через **Radmin VPN**.

## Что это

Два .exe файла:
- **KenshiLauncher.Host.exe** — лаунчер хоста. Запускает выделенный сервер Kenshi и показывает ваш IP для друзей.
- **KenshiLauncher.Client.exe** — лаунчер клиента. Подключается к серверу и запускает игру.

Внутри используется проект [Kenshi-Online](https://github.com/The404Studios/Kenshi-Online) — open-source мультиплеерный мод для Kenshi.

## Требования

- Windows 10/11 (x64)
- Steam версия Kenshi 1.0.68+
- [Radmin VPN](https://www.radmin-vpn.com/) — для создания локальной сети между игроками

## Как играть

### Шаг 1 — Настройка Radmin VPN

1. Скачайте и установите [Radmin VPN](https://www.radmin-vpn.com/)
2. Создайте сеть (или присоединитесь к существующей)
3. Добавьте всех игроков в одну сеть

### Шаг 2 — Хост (создатель мира)

1. Скачайте `KenshiLauncher.Host.exe` из [Releases](../../releases)
2. Запустите от имени администратора
3. Нажмите **"Установить мод"** — лаунчер автоматически скачает Kenshi-Online
4. Настройте сервер: имя, порт, макс. игроков (до 5), PvP
5. Нажмите **"Запустить сервер"**
6. Скопируйте IP:Port (кнопка "Копировать IP") и отправьте друзьям

### Шаг 3 — Клиенты (остальные игроки)

1. Скачайте `KenshiLauncher.Client.exe` из [Releases](../../releases)
2. Запустите, нажмите **"Установить мод"**
3. Введите своё имя игрока
4. Введите IP:Port от хоста (полученный на шаге 2)
5. Нажмите **"Играть"**
6. В главном меню Kenshi нажмите **MULTIPLAYER** или кнопку **\`** (Ё) для внутриигрового меню подключения

### Управление в игре

| Клавиша | Действие |
|---------|---------|
| \` (Ё/тильда) | Открыть меню мультиплеера |
| F1 | Переключить HUD |
| Enter | Чат |
| Tab | Список игроков |

## Сборка из исходников

### Требования

- Windows 10/11 (x64)
- Visual Studio 2022 с C++ Desktop workload
- CMake 3.20+

### Команды

```powershell
# Клонировать
git clone --recursive https://github.com/YOUR_USER/KenshiNetworkLauncher.git
cd KenshiNetworkLauncher

# Сборка
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
cmake --build build --config Release

# Результат в
# build\HostLauncher\Release\KenshiLauncher.Host.exe
# build\ClientLauncher\Release\KenshiLauncher.Client.exe
```

## Структура проекта

```
src/
  LauncherCommon/   — общий код (поиск игры, скачивание, деплой)
  HostLauncher/      — exe хоста
  ClientLauncher/    — exe клиента
```

## Совместимость

| Компонент | Версия |
|-----------|--------|
| Игра | Kenshi 1.0.68 (Steam) |
| Сеть | Radmin VPN (подсети 100.x.x.x / 10.x.x.x) |
| Макс. игроков | 5 (настраивается) |
| Порт по умолчанию | 27800 UDP |

## Лицензия

MIT License — свободное использование и модификация.
Kenshi-Online лицензирован отдельно (MIT).