#include "ClientWindow.h"
#include "../LauncherCommon/LauncherCommon.h"
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <thread>
#include <vector>
#include <cstdio>

using namespace LauncherCommon;

#pragma comment(lib, "comctl32.lib")

#ifndef PBM_SETPOS
#define PBM_SETPOS 0x0402
#endif

static HWND g_hwnd = nullptr;
static HWND g_hProgress = nullptr;
static HWND g_hInstallBtn = nullptr;
static HWND g_hPlayBtn = nullptr;
static HWND g_hStatus = nullptr;
static std::wstring g_kenshiPath;
static bool g_modInstalled = false;

#define WM_INSTALL_COMPLETE (WM_USER + 10)
#define WM_INSTALL_ERROR   (WM_USER + 11)

static void SetStatus(HWND hwnd, const wchar_t* txt) {
    if (!hwnd) return;
    SetDlgItemTextW(hwnd, 2, txt);
}

static void SetProgress(HWND hwnd, int p) {
    if (!hwnd || !g_hProgress) return;
    SendMessageW(g_hProgress, PBM_SETPOS, p, 0);
}

static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG: {
        g_hwnd = hwnd;
        g_hProgress = GetDlgItem(hwnd, 101);
        g_hInstallBtn = GetDlgItem(hwnd, 3);
        g_hPlayBtn = GetDlgItem(hwnd, 4);
        g_hStatus = GetDlgItem(hwnd, 2);

        SetWindowPos(hwnd, nullptr, 0, 0, 440, 400, SWP_NOMOVE | SWP_NOZORDER);

        SendMessageW(g_hProgress, PBM_SETRANGE, 0, MAKEPARAM(0, 100));
        SendMessageW(g_hProgress, PBM_SETPOS, 0, 0);

        // Ищем Kenshi
        g_kenshiPath = LauncherCommon::FindKenshiPath();
        if (!g_kenshiPath.empty()) {
            wchar_t buf[512];
            swprintf(buf, L"Путь: %s", g_kenshiPath.c_str());
            SetDlgItemTextW(hwnd, 1, buf);

            g_modInstalled = LauncherCommon::IsModInstalled(g_kenshiPath);
            if (g_modInstalled) {
                SetDlgItemTextW(hwnd, 2, L"[✓] Kenshi-Online мод установлен");
                EnableWindow(g_hPlayBtn, TRUE);
            } else {
                SetDlgItemTextW(hwnd, 2, L"[—] Kenshi-Online мод не установлен");
            }
        } else {
            SetDlgItemTextW(hwnd, 1, L" Kenshi не найден. Установите игру в Steam.");
            SetDlgItemTextW(hwnd, 2, L"[✗] Установите Kenshi в Steam и перезапустите лаунчер.");
        }

        return TRUE;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);

        if (id == 3) { // Установить мод
            if (g_kenshiPath.empty()) {
                MessageBoxW(hwnd, L"Kenshi не найден.", L"Ошибка", MB_ICONERROR);
                return TRUE;
            }

            EnableWindow(g_hInstallBtn, FALSE);
            SetStatus(hwnd, L"Скачивание Kenshi-Online...");
            SetProgress(hwnd, 10);

            std::thread([hwnd]() {
                LatestRelease release;
                if (!FetchLatestRelease("The404Studios", "Kenshi-Online", release)) {
                    PostMessageW(hwnd, WM_INSTALL_ERROR, 0, (LPARAM)L"Не удалось получить информацию о релизе.");
                    return;
                }

                wchar_t buf[256];
                swprintf(buf, L"Релиз: %S", release.version.c_str());
                SetStatus(hwnd, buf);
                SetProgress(hwnd, 30);

                std::wstring modPath = DownloadAndExtract(release.zipUrl, g_kenshiPath);
                if (modPath.empty()) {
                    PostMessageW(hwnd, WM_INSTALL_ERROR, 0, (LPARAM)L"Ошибка скачивания.");
                    return;
                }

                SetProgress(hwnd, 70);
                std::wstring err;
                DeployResult res = DeployModFiles(g_kenshiPath, modPath, &err);
                SetProgress(hwnd, 100);

                if (res == DeployResult::Success) {
                    g_modInstalled = true;
                    PostMessageW(hwnd, WM_INSTALL_COMPLETE, 0, 0);
                } else {
                    wchar_t errBuf[512];
                    swprintf(errBuf, L"Ошибка установки: %d", (int)res);
                    PostMessageW(hwnd, WM_INSTALL_ERROR, 0, (LPARAM)errBuf);
                }
            }).detach();
            return TRUE;
        }

        if (id == 4) { // Играть
            if (!g_modInstalled) {
                MessageBoxW(hwnd, L"Сначала установите мод.", L"Ошибка", MB_ICONERROR);
                return TRUE;
            }

            // Читаем имя игрока и адрес сервера
            wchar_t playerName[64], serverAddr[128];
            GetDlgItemTextW(hwnd, 103, playerName, 64);
            GetDlgItemTextW(hwnd, 104, serverAddr, 128);

            if (wcslen(playerName) < 1) {
                MessageBoxW(hwnd, L"Введите имя игрока.", L"Ошибка", MB_ICONWARNING);
                return TRUE;
            }
            if (wcslen(serverAddr) < 1) {
                MessageBoxW(hwnd, L"Введите адрес сервера (IP:Port).", L"Ошибка", MB_ICONWARNING);
                return TRUE;
            }

            SetStatus(hwnd, L"Запуск Kenshi...");
            EnableWindow(g_hPlayBtn, FALSE);
            EnableWindow(g_hInstallBtn, FALSE);

            // Сохраняем параметры в settings.txt для передачи в игру
            std::wstring settingsPath = g_kenshiPath + L"\\data\\config\\KenshiMP_Client.cfg";
            FILE* f = _wfopen(settingsPath.c_str(), L"w, ccs=UTF-8");
            if (f) {
                fwprintf(f, L"playerName=%s\nserverAddress=%s\n", playerName, serverAddr);
                fclose(f);
            }

            // Запускаем Kenshi
            std::wstring kenshiExe = g_kenshiPath + L"\\kenshi_x64.exe";
            if (GetFileAttributesW(kenshiExe.c_str()) == INVALID_FILE_ATTRIBUTES) {
                MessageBoxW(hwnd, L"kenshi_x64.exe не найден.", L"Ошибка", MB_ICONERROR);
                return TRUE;
            }

            // Запускаем через KenshiMP.Injector если есть, иначе напрямую
            std::wstring injectorPath = g_kenshiPath + L"\\KenshiMP.Injector.exe";
            std::wstring cmd;
            if (GetFileAttributesW(injectorPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                cmd = injectorPath;
            } else {
                cmd = kenshiExe;
            }

            STARTUPINFOW si = {};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi = {};
            if (!CreateProcessW(cmd.c_str(), nullptr, nullptr, nullptr,
                    FALSE, CREATE_DEFAULT_ERROR_MODE, nullptr,
                    g_kenshiPath.c_str(), &si, &pi)) {
                MessageBoxW(hwnd, L"Не удалось запустить игру.", L"Ошибка", MB_ICONERROR);
                SetStatus(hwnd, L"Ошибка запуска");
                EnableWindow(g_hPlayBtn, TRUE);
                EnableWindow(g_hInstallBtn, TRUE);
                return TRUE;
            }

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            SetStatus(hwnd, L"Kenshi запускается...");
            return TRUE;
        }

        if (id == 5) { // Выход
            EndDialog(hwnd, 0);
            return TRUE;
        }
        return FALSE;
    }

    case WM_INSTALL_COMPLETE:
        SetStatus(hwnd, L"[✓] Kenshi-Online мод установлен!");
        SetProgress(hwnd, 100);
        EnableWindow(g_hInstallBtn, FALSE);
        EnableWindow(g_hPlayBtn, TRUE);
        g_modInstalled = true;
        MessageBoxW(hwnd,
            L"Мод установлен!\n\nВведите адрес сервера (IP:Port от хоста) и нажмите 'Играть'.",
            L"Установка завершена", MB_ICONINFORMATION);
        return TRUE;

    case WM_INSTALL_ERROR: {
        const wchar_t* err = (const wchar_t*)lParam;
        wchar_t msg[512];
        swprintf(msg, L"Ошибка: %s\n\nПроверьте интернет-соединение и перезапустите лаунчер.", err);
        MessageBoxW(hwnd, msg, L"Ошибка", MB_ICONERROR);
        SetStatus(hwnd, L"Ошибка установки");
        SetProgress(hwnd, 0);
        EnableWindow(g_hInstallBtn, TRUE);
        return TRUE;
    }

    case WM_CLOSE:
        EndDialog(hwnd, 0);
        return TRUE;
    }
    return FALSE;
}

int RunClientWindow(HINSTANCE hInstance) {
    InitCommonControls();
    return (int)DialogBoxParamW(hInstance, MAKEINTRESOURCEW(1000), nullptr, DlgProc, 0);
}

void ClientWindowSetModInstalled(HWND hwnd) { SendMessageW(hwnd, WM_INSTALL_COMPLETE, 0, 0); }
void ClientWindowSetProgress(HWND hwnd, int p) { SetProgress(hwnd, p); }
void ClientWindowSetStatus(HWND hwnd, const wchar_t* status) { SetStatus(hwnd, status); }
void ClientWindowEnablePlay(HWND hwnd, bool enable) { EnableWindow(GetDlgItem(hwnd, 4), enable); }
void ClientWindowNotifyError(HWND hwnd, const wchar_t* err) { SendMessageW(hwnd, WM_INSTALL_ERROR, 0, (LPARAM)err); }