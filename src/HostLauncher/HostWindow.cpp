#include "HostWindow.h"
#include "../LauncherCommon/LauncherCommon.h"
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>
#include <cstring>
#include <functional>
#include <memory>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")

// Структура конфигурации сервера
struct ServerConfig {
    std::wstring serverName = L"Kenshi Server";
    int port = 27800;
    int maxPlayers = 5;
    bool pvpEnabled = true;
    std::wstring kenshiPath;
    std::wstring kenshiVersion;
    bool modInstalled = false;
};

// Глобальные данные окна
static HWND g_hwnd = nullptr;
static HWND g_hLog = nullptr;
static HWND g_hStatus = nullptr;
static HWND g_hInstallBtn = nullptr;
static HWND g_hLaunchBtn = nullptr;
static HWND g_hStopBtn = nullptr;
static HWND g_hProgress = nullptr;
static HWND g_hPortEdit = nullptr;
static HWND g_hNameEdit = nullptr;
static HWND g_hPlayersSlider = nullptr;
static HWND g_hPlayersLabel = nullptr;
static HWND g_hPvPCheck = nullptr;
static HWND g_hIPLabel = nullptr;
static bool g_serverRunning = false;
static ServerConfig g_config;
static std::mutex g_mutex;

static std::wstring g_kenshiPath;
static std::wstring g_localIP;
static int g_localPort = 27800;

static void AddLog(HWND hwnd, const wchar_t* msg) {
    if (!hwnd) return;
    HWND log = GetDlgItem(hwnd, 100);
    if (!log) return;
    int count = SendMessageW(log, LB_GETCOUNT, 0, 0);
    SendMessageW(log, LB_ADDSTRING, 0, (LPARAM)msg);
    SendMessageW(log, LB_SETCURSEL, count, 0);
}

static void SetStatus(HWND hwnd, const wchar_t* status) {
    if (!hwnd) return;
    HWND s = GetDlgItem(hwnd, 200);
    if (s) SetWindowTextW(s, status);
}

static void SetProgress(HWND hwnd, int progress) {
    if (!hwnd) return;
    HWND p = GetDlgItem(hwnd, 101);
    if (p) SendMessageW(p, PBM_SETPOS, progress, 0);
}

static void EnableControls(HWND hwnd, bool enable) {
    EnableWindow(GetDlgItem(hwnd, 102), enable); // port
    EnableWindow(GetDlgItem(hwnd, 103), enable); // name
    EnableWindow(GetDlgItem(hwnd, 104), enable); // slider
    EnableWindow(GetDlgItem(hwnd, 105), enable); // pvp
}

static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG: {
        g_hwnd = hwnd;

        // Стиль окна
        SetWindowPos(hwnd, nullptr, 0, 0, 520, 620, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowTextW(hwnd, L"Kenshi — Лаунчер хоста");

        // Ищем Kenshi
        g_kenshiPath = LauncherCommon::FindKenshiPath();
        g_localIP = LauncherCommon::GetLocalVPNIP();
        if (g_localIP.empty()) g_localIP = LauncherCommon::GetLocalIP();

        wchar_t buf[256];
        if (!g_kenshiPath.empty()) {
            swprintf(buf, L"Путь: %s", g_kenshiPath.c_str());
            g_config.kenshiPath = g_kenshiPath;
            g_config.modInstalled = LauncherCommon::IsModInstalled(g_kenshiPath);
        } else {
            wcscpy(buf, L" Kenshi не найден. Установите игру в Steam.");
        }
        SetDlgItemTextW(hwnd, 1, buf);

        // Статус мода
        if (g_config.modInstalled) {
            SetDlgItemTextW(hwnd, 2, L"[✓] Kenshi-Online мод установлен");
        } else {
            SetDlgItemTextW(hwnd, 2, L"[—] Kenshi-Online мод не установлен");
        }

        // IP
        swprintf(buf, L"Ваш IP (VPN): %s", g_localIP.c_str());
        SetDlgItemTextW(hwnd, 300, buf);
        g_hIPLabel = GetDlgItem(hwnd, 300);

        // Начальные значения
        SetDlgItemTextW(hwnd, 103, g_config.serverName.c_str());
        wchar_t portBuf[16];
        swprintf(portBuf, L"%d", g_config.port);
        SetDlgItemTextW(hwnd, 102, portBuf);
        SendMessageW(GetDlgItem(hwnd, 104), TBM_SETRANGE, TRUE, MAKELONG(1, 5));
        SendMessageW(GetDlgItem(hwnd, 104), TBM_SETPOS, TRUE, g_config.maxPlayers);
        swprintf(buf, L"Макс. игроков: %d", g_config.maxPlayers);
        SetDlgItemTextW(hwnd, 106, buf);
        Button_SetCheck(GetDlgItem(hwnd, 105), g_config.pvpEnabled ? BST_CHECKED : BST_UNCHECKED);

        // Кнопки
        g_hInstallBtn = GetDlgItem(hwnd, 3);
        g_hLaunchBtn = GetDlgItem(hwnd, 4);
        g_hStopBtn = GetDlgItem(hwnd, 5);
        EnableWindow(g_hLaunchBtn, g_config.modInstalled ? TRUE : FALSE);
        EnableWindow(g_hStopBtn, FALSE);

        // Прогресс бар
        g_hProgress = GetDlgItem(hwnd, 101);
        SendMessageW(g_hProgress, PBM_SETRANGE, 0, MAKEPARAM(0, 100));
        SendMessageW(g_hProgress, PBM_SETPOS, 0, 0);

        AddLog(hwnd, L"=== Kenshi Network Launcher ===");
        AddLog(hwnd, L"Если Kenshi не найден — установите игру в Steam и перезапустите лаунчер.");
        if (!g_kenshiPath.empty() && !g_config.modInstalled) {
            AddLog(hwnd, L"Нажмите 'Установить мод' для загрузки Kenshi-Online.");
        } else if (!g_kenshiPath.empty()) {
            AddLog(hwnd, L"Мод установлен. Нажмите 'Запустить сервер'.");
        }
        return TRUE;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);

        if (id == 3) { // Установить мод
            if (g_kenshiPath.empty()) {
                MessageBoxW(hwnd, L"Kenshi не найден. Проверьте установку игры.", L"Ошибка", MB_ICONERROR);
                return TRUE;
            }

            EnableWindow(g_hInstallBtn, FALSE);
            SetStatus(hwnd, L"Скачивание Kenshi-Online...");
            SetProgress(hwnd, 10);
            AddLog(hwnd, L"Подключение к GitHub...");

            std::thread([hwnd]() {
                LatestRelease release;
                bool ok = FetchLatestRelease("The404Studios", "Kenshi-Online", release);

                if (!ok) {
                    PostMessageW(hwnd, WM_USER + 1, 2, (LPARAM)L"Ошибка: не удалось получить информацию о релизе. Проверьте интернет-соединение.");
                    return;
                }

                wchar_t logBuf[256];
                swprintf(logBuf, L"Найден релиз: %S", release.version.c_str());
                AddLog(hwnd, logBuf);
                SetProgress(hwnd, 30);

                AddLog(hwnd, L"Скачивание архива...");
                std::wstring modPath = DownloadAndExtract(release.zipUrl, g_kenshiPath);
                SetProgress(hwnd, 80);

                if (modPath.empty()) {
                    PostMessageW(hwnd, WM_USER + 1, 2, (LPARAM)L"Ошибка: не удалось скачать мод. Проверьте интернет-соединение.");
                    return;
                }

                AddLog(hwnd, L"Установка файлов мода...");
                std::wstring err;
                DeployResult res = DeployModFiles(g_kenshiPath, modPath, &err);
                SetProgress(hwnd, 100);

                if (res == DeployResult::Success) {
                    PostMessageW(hwnd, WM_USER + 1, 3, 0); // install complete
                } else if (res == DeployResult::KenshiNotFound) {
                    PostMessageW(hwnd, WM_USER + 1, 2, (LPARAM)L"Ошибка: папка Kenshi не найдена.");
                } else {
                    wchar_t errBuf[512];
                    swprintf(errBuf, L"Ошибка установки: %d", (int)res);
                    PostMessageW(hwnd, WM_USER + 1, 2, (LPARAM)errBuf);
                }
            }).detach();
            return TRUE;
        }

        if (id == 4) { // Запустить сервер
            if (!g_config.modInstalled) {
                MessageBoxW(hwnd, L"Сначала установите мод.", L"Ошибка", MB_ICONERROR);
                return TRUE;
            }

            // Читаем настройки
            wchar_t nameBuf[128], portBuf[16];
            GetDlgItemTextW(hwnd, 103, nameBuf, 128);
            GetDlgItemTextW(hwnd, 102, portBuf, 16);
            int port = _wtoi(portBuf);
            if (port <= 0 || port > 65535) port = 27800;
            int players = SendMessageW(GetDlgItem(hwnd, 104), TBM_GETPOS, 0, 0);
            bool pvp = Button_GetCheck(GetDlgItem(hwnd, 105)) == BST_CHECKED;

            g_config.serverName = nameBuf;
            g_config.port = port;
            g_config.maxPlayers = players;
            g_config.pvpEnabled = pvp;
            g_localPort = port;

            EnableControls(hwnd, false);
            EnableWindow(g_hLaunchBtn, FALSE);
            EnableWindow(g_hStopBtn, TRUE);
            EnableWindow(g_hInstallBtn, FALSE);
            SetStatus(hwnd, L"Запуск сервера...");

            AddLog(hwnd, L"Генерация server.json...");
            AddLog(hwnd, L"Запуск KenshiMP.Server.exe...");

            std::thread([hwnd, port, players, pvp]() {
                // Генерируем server.json
                std::wstring jsonPath = g_kenshiPath + L"\\server.json";
                FILE* f = _wfopen(jsonPath.c_str(), L"w, ccs=UTF-8");
                if (f) {
                    fwprintf(f, L"{\n");
                    fwprintf(f, L"  \"serverName\": \"%s\",\n", g_config.serverName.c_str());
                    fwprintf(f, L"  \"port\": %d,\n", port);
                    fwprintf(f, L"  \"maxPlayers\": %d,\n", players);
                    fwprintf(f, L"  \"pvpEnabled\": %s,\n", pvp ? L"true" : L"false");
                    fwprintf(f, L"  \"gameSpeed\": 1.0,\n");
                    fwprintf(f, L"  \"savePath\": \"world.kmpsave\",\n");
                    fwprintf(f, L"  \"tickRate\": 20,\n");
                    fwprintf(f, L"  \"masterServer\": \"\",\n");
                    fwprintf(f, L"  \"masterPort\": 27801,\n");
                    fwprintf(f, L"  \"password\": \"\"\n");
                    fwprintf(f, L"}\n");
                    fclose(f);
                    AddLog(hwnd, L"server.json создан.");
                } else {
                    AddLog(hwnd, L"Ошибка: не удалось создать server.json");
                    PostMessageW(hwnd, WM_USER + 1, 4, 0);
                    return;
                }

                // Запускаем сервер
                std::wstring serverExe = g_kenshiPath + L"\\KenshiMP.Server.exe";
                if (GetFileAttributesW(serverExe.c_str()) == INVALID_FILE_ATTRIBUTES) {
                    AddLog(hwnd, L"Ошибка: KenshiMP.Server.exe не найден");
                    PostMessageW(hwnd, WM_USER + 1, 2, (LPARAM)L"Ошибка: KenshiMP.Server.exe не найден. Установите мод заново.");
                    PostMessageW(hwnd, WM_USER + 1, 4, 0);
                    return;
                }

                STARTUPINFOW si = {};
                si.cb = sizeof(si);
                si.dwFlags = STARTF_USESTDHANDLES;
                PROCESS_INFORMATION pi = {};
                SECURITY_ATTRIBUTES sa = {};
                sa.bInheritHandle = TRUE;

                HANDLE hOutR, hOutW;
                CreatePipe(&hOutR, &hOutW, &sa, 0);
                si.hStdOutput = hOutW;
                si.hStdError = hOutW;

                std::wstring workDir = g_kenshiPath;
                BOOL created = CreateProcessW(serverExe.c_str(), nullptr,
                    nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                    workDir.c_str(), &si, &pi);

                if (!created) {
                    AddLog(hwnd, L"Ошибка: не удалось запустить сервер");
                    PostMessageW(hwnd, WM_USER + 1, 2, (LPARAM)L"Не удалось запустить KenshiMP.Server.exe");
                    PostMessageW(hwnd, WM_USER + 1, 4, 0);
                    CloseHandle(hOutR);
                    CloseHandle(hOutW);
                    return;
                }

                CloseHandle(hOutW);
                g_serverRunning = true;

                // Читаем вывод сервера в цикле
                char lineBuf[1024];
                DWORD bytesRead;
                while (g_serverRunning) {
                    if (ReadFile(hOutR, lineBuf, sizeof(lineBuf) - 1, &bytesRead, nullptr) && bytesRead > 0) {
                        lineBuf[bytesRead] = '\0';
                        for (char* p = lineBuf; *p; ++p) {
                            if (*p == '\n' || *p == '\r') {
                                if (p > lineBuf && *(p - 1) != '\n' && *(p - 1) != '\r') {
                                    *p = '\0';
                                    std::string s(lineBuf);
                                    std::wstring ws(s.begin(), s.end());
                                    AddLog(hwnd, ws.c_str());
                                    p++;
                                    memmove(lineBuf, p, strlen(p) + 1);
                                    p = lineBuf - 1;
                                }
                            }
                        }
                    } else {
                        break;
                    }

                    DWORD code;
                    if (!GetExitCodeProcess(pi.hProcess, &code) || code != STILL_ACTIVE)
                        break;
                }

                AddLog(hwnd, L"Сервер остановлен.");
                PostMessageW(hwnd, WM_USER + 1, 4, 0);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                CloseHandle(hOutR);
            }).detach();

            return TRUE;
        }

        if (id == 5) { // Остановить сервер
            g_serverRunning = false;
            AddLog(hwnd, L"Остановка сервера...");
            EnableWindow(g_hStopBtn, FALSE);
            SetStatus(hwnd, L"Сервер останавливается...");
            return TRUE;
        }

        if (id == 104 + 1000) { // Slider scroll
            int pos = SendMessageW((HWND)lParam, TBM_GETPOS, 0, 0);
            wchar_t buf[64];
            swprintf(buf, L"Макс. игроков: %d", pos);
            SetDlgItemTextW(hwnd, 106, buf);
            return TRUE;
        }

        if (id == 6) { // Выход
            g_serverRunning = false;
            EndDialog(hwnd, 0);
            return TRUE;
        }

        if (id == 7) { // Копировать IP
            if (g_localIP.empty()) return TRUE;
            wchar_t buf[128];
            swprintf(buf, L"%s:%d", g_localIP.c_str(), g_localPort);
            if (OpenClipboard(nullptr)) {
                EmptyClipboard();
                HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, (wcslen(buf) + 1) * sizeof(wchar_t));
                if (h) {
                    wchar_t* p = (wchar_t*)GlobalLock(h);
                    wcscpy(p, buf);
                    GlobalUnlock(h);
                    SetClipboardData(CF_UNICODETEXT, h);
                }
                CloseClipboard();
            }
            AddLog(hwnd, L"IP скопирован в буфер обмена!");
            return TRUE;
        }

        return FALSE;
    }

    case WM_USER + 1: {
        int type = (int)wParam;
        const wchar_t* msg = (const wchar_t*)lParam;

        switch (type) {
        case 2: // Error
            MessageBoxW(hwnd, msg, L"Ошибка", MB_ICONERROR);
            EnableWindow(g_hInstallBtn, TRUE);
            SetProgress(hwnd, 0);
            SetStatus(hwnd, L"Ошибка");
            break;
        case 3: // Install complete
            g_config.modInstalled = true;
            SetDlgItemTextW(hwnd, 2, L"[✓] Kenshi-Online мод установлен");
            SetStatus(hwnd, L"Мод установлен!");
            SetProgress(hwnd, 100);
            EnableWindow(g_hInstallBtn, FALSE);
            EnableWindow(g_hLaunchBtn, TRUE);
            AddLog(hwnd, L"=== Установка завершена ===");
            AddLog(hwnd, L"Нажмите 'Запустить сервер' для начала игры.");
            break;
        case 4: // Server stopped
            EnableControls(hwnd, true);
            EnableWindow(g_hInstallBtn, TRUE);
            EnableWindow(g_hLaunchBtn, TRUE);
            EnableWindow(g_hStopBtn, FALSE);
            SetStatus(hwnd, L"Сервер остановлен");
            g_serverRunning = false;
            break;
        }
        return TRUE;
    }

    case WM_CLOSE:
        g_serverRunning = false;
        EndDialog(hwnd, 0);
        return TRUE;
    }
    return FALSE;
}

HWND CreateHostWindow(HINSTANCE hInstance, HostWindowCallbacks callbacks) {
    (void)callbacks;
    DialogBoxParamW(hInstance, MAKEINTRESOURCEW(1000), nullptr, DlgProc, 0);
    return nullptr;
}

int RunHostWindow(HINSTANCE hInstance) {
    return (int)DialogBoxParamW(hInstance, MAKEINTRESOURCEW(1000), nullptr, DlgProc, 0);
}

void HostWindowSetStatus(HWND hwnd, const wchar_t* status) { SetStatus(hwnd, status); }
void HostWindowSetProgress(HWND hwnd, int progress) { SetProgress(hwnd, progress); }
void HostWindowAddLog(HWND hwnd, const wchar_t* line) { AddLog(hwnd, line); }
void HostWindowSetServerRunning(HWND hwnd, bool running) {
    EnableWindow(GetDlgItem(hwnd, 4), running ? FALSE : TRUE);
    EnableWindow(GetDlgItem(hwnd, 5), running ? TRUE : FALSE);
}
void HostWindowSetServerIP(HWND hwnd, const wchar_t* ip, int port) {
    wchar_t buf[128];
    swprintf(buf, L"Ваш IP (VPN): %s:%d", ip, port);
    SetDlgItemTextW(hwnd, 300, buf);
}
void HostWindowNotifyInstallComplete(HWND hwnd) {
    PostMessageW(hwnd, WM_USER + 1, 3, 0);
}
void HostWindowNotifyError(HWND hwnd, const wchar_t* err) {
    PostMessageW(hwnd, WM_USER + 1, 2, (LPARAM)err);
}
