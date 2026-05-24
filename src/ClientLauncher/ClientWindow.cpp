#include "ClientWindow.h"
#include "../LauncherCommon/LauncherCommon.h"
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <string>
#include <thread>
#include <vector>
#include <cstdio>

using namespace LauncherCommon;

#pragma comment(lib, "comctl32.lib")

#ifndef PBM_SETPOS
#define PBM_SETPOS 0x0402
#endif
#ifndef MAKEPARAM
#define MAKEPARAM(a, b) ((LPARAM)MAKELONG(a, b))
#endif

static HWND g_hwnd = nullptr;
static HWND g_hProgress = nullptr;
static HWND g_hInstallBtn = nullptr;
static HWND g_hPlayBtn = nullptr;
static HWND g_hStatus = nullptr;
static HWND g_hPathCombo = nullptr;
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

static bool VerifyKenshiPath(const std::wstring& path) {
    if (path.empty()) return false;
    std::wstring exe = path + L"\\kenshi_x64.exe";
    return GetFileAttributesW(exe.c_str()) != INVALID_FILE_ATTRIBUTES;
}

static void UpdateModStatus(HWND hwnd, const std::wstring& path) {
    if (!hwnd) return;
    if (path.empty()) {
        SetStatus(hwnd, L"[--] Kenshi not found. Select the Kenshi folder manually.");
        EnableWindow(g_hInstallBtn, FALSE);
        EnableWindow(g_hPlayBtn, FALSE);
        return;
    }
    if (!VerifyKenshiPath(path)) {
        SetStatus(hwnd, L"[X] Invalid folder — kenshi_x64.exe not found.");
        EnableWindow(g_hInstallBtn, FALSE);
        EnableWindow(g_hPlayBtn, FALSE);
        return;
    }
    g_kenshiPath = path;
    bool installed = LauncherCommon::IsModInstalled(path);
    g_modInstalled = installed;
    if (installed) {
        SetStatus(hwnd, L"[OK] Kenshi-Online mod is installed.");
        EnableWindow(g_hInstallBtn, FALSE);
        EnableWindow(g_hPlayBtn, TRUE);
    } else {
        SetStatus(hwnd, L"[--] Kenshi-Online mod not installed.");
        EnableWindow(g_hInstallBtn, TRUE);
        EnableWindow(g_hPlayBtn, FALSE);
    }
}

static void SetKenshiPath(HWND hwnd, const std::wstring& path) {
    g_kenshiPath = path;
    if (!path.empty()) {
        SendMessageW(g_hPathCombo, CB_RESETCONTENT, 0, 0);
        SendMessageW(g_hPathCombo, CB_ADDSTRING, 0, (LPARAM)path.c_str());
        SendMessageW(g_hPathCombo, CB_SETCURSEL, 0, 0);
    }
    UpdateModStatus(hwnd, path);
}

static void BrowseFolder(HWND hwnd) {
    wchar_t buf[MAX_PATH] = {};
    BROWSEINFOW bi = {};
    bi.lpszTitle = L"Select the Kenshi game folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        if (SHGetPathFromIDListW(pidl, buf)) {
            SetKenshiPath(hwnd, std::wstring(buf));
        }
        CoTaskMemFree(pidl);
    }
}

static std::wstring GetComboText(HWND hCombo) {
    wchar_t buf[MAX_PATH * 2] = {};
    SendMessageW(hCombo, WM_GETTEXT, MAX_PATH * 2, (LPARAM)buf);
    return std::wstring(buf);
}

static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG: {
        g_hwnd = hwnd;
        g_hProgress = GetDlgItem(hwnd, 101);
        g_hInstallBtn = GetDlgItem(hwnd, 3);
        g_hPlayBtn = GetDlgItem(hwnd, 4);
        g_hStatus = GetDlgItem(hwnd, 2);
        g_hPathCombo = GetDlgItem(hwnd, 1);

        SetWindowPos(hwnd, nullptr, 0, 0, 480, 490, SWP_NOMOVE | SWP_NOZORDER);

        SendMessageW(g_hProgress, PBM_SETRANGE, 0, MAKEPARAM(0, 100));
        SendMessageW(g_hProgress, PBM_SETPOS, 0, 0);

        std::wstring autoPath = LauncherCommon::FindKenshiPath();
        if (!autoPath.empty()) {
            SetKenshiPath(hwnd, autoPath);
        } else {
            SetStatus(hwnd, L"[--] Kenshi not found. Click 'Browse...' to select the folder.");
            EnableWindow(g_hInstallBtn, FALSE);
            EnableWindow(g_hPlayBtn, FALSE);
        }
        return TRUE;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        if (id == 10 && code == BN_CLICKED) {
            BrowseFolder(hwnd);
            return TRUE;
        }

        if (id == 1 && code == CBN_EDITCHANGE) {
            std::wstring path = GetComboText(g_hPathCombo);
            if (!path.empty()) {
                SetKenshiPath(hwnd, path);
            }
            return TRUE;
        }

        if (id == 1 && code == CBN_SELCHANGE) {
            int sel = SendMessageW(g_hPathCombo, CB_GETCURSEL, 0, 0);
            if (sel != CB_ERR) {
                wchar_t buf[MAX_PATH * 2] = {};
                SendMessageW(g_hPathCombo, CB_GETLBTEXT, sel, (LPARAM)buf);
                SetKenshiPath(hwnd, std::wstring(buf));
            }
            return TRUE;
        }

        if (id == 3) {
            if (g_kenshiPath.empty() || !VerifyKenshiPath(g_kenshiPath)) {
                MessageBoxW(hwnd, L"Select the Kenshi game folder first.", L"Error", MB_ICONERROR);
                return TRUE;
            }

            EnableWindow(g_hInstallBtn, FALSE);
            SetStatus(hwnd, L"Downloading Kenshi-Online...");
            SetProgress(hwnd, 10);

            std::thread([hwnd]() {
                LatestRelease release;
                if (!FetchLatestRelease("The404Studios", "Kenshi-Online", release)) {
                    PostMessageW(hwnd, WM_INSTALL_ERROR, 0, (LPARAM)L"Failed to fetch release info.");
                    return;
                }

                wchar_t buf[256];
                swprintf(buf, L"Release: %S", release.version.c_str());
                SetStatus(hwnd, buf);
                SetProgress(hwnd, 30);

                std::wstring modPath = DownloadAndExtract(release.zipUrl, g_kenshiPath);
                if (modPath.empty()) {
                    PostMessageW(hwnd, WM_INSTALL_ERROR, 0, (LPARAM)L"Download failed.");
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
                    swprintf(errBuf, L"Install error: %d — %S", (int)res, err.c_str());
                    PostMessageW(hwnd, WM_INSTALL_ERROR, 0, (LPARAM)errBuf);
                }
            }).detach();
            return TRUE;
        }

        if (id == 4) {
            if (!g_modInstalled) {
                MessageBoxW(hwnd, L"Install the mod first.", L"Error", MB_ICONERROR);
                return TRUE;
            }

            wchar_t playerName[64], serverAddr[128];
            GetDlgItemTextW(hwnd, 103, playerName, 64);
            GetDlgItemTextW(hwnd, 104, serverAddr, 128);

            if (wcslen(playerName) < 1) {
                MessageBoxW(hwnd, L"Enter your player name.", L"Error", MB_ICONWARNING);
                return TRUE;
            }
            if (wcslen(serverAddr) < 1) {
                MessageBoxW(hwnd, L"Enter server address (IP:Port).", L"Error", MB_ICONWARNING);
                return TRUE;
            }

            SetStatus(hwnd, L"Starting Kenshi...");
            EnableWindow(g_hPlayBtn, FALSE);
            EnableWindow(g_hInstallBtn, FALSE);

            std::wstring settingsPath = g_kenshiPath + L"\\data\\config\\KenshiMP_Client.cfg";
            FILE* f = _wfopen(settingsPath.c_str(), L"w, ccs=UTF-8");
            if (f) {
                fwprintf(f, L"playerName=%s\nserverAddress=%s\n", playerName, serverAddr);
                fclose(f);
            }

            std::wstring kenshiExe = g_kenshiPath + L"\\kenshi_x64.exe";
            if (GetFileAttributesW(kenshiExe.c_str()) == INVALID_FILE_ATTRIBUTES) {
                MessageBoxW(hwnd, L"kenshi_x64.exe not found.", L"Error", MB_ICONERROR);
                return TRUE;
            }

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
                MessageBoxW(hwnd, L"Failed to start game.", L"Error", MB_ICONERROR);
                SetStatus(hwnd, L"Start failed");
                EnableWindow(g_hPlayBtn, TRUE);
                EnableWindow(g_hInstallBtn, TRUE);
                return TRUE;
            }

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            SetStatus(hwnd, L"Kenshi is starting...");
            return TRUE;
        }

        if (id == 5) {
            EndDialog(hwnd, 0);
            return TRUE;
        }
        return FALSE;
    }

    case WM_INSTALL_COMPLETE:
        SetStatus(hwnd, L"[OK] Kenshi-Online mod installed!");
        SetProgress(hwnd, 100);
        EnableWindow(g_hInstallBtn, FALSE);
        EnableWindow(g_hPlayBtn, TRUE);
        g_modInstalled = true;
        MessageBoxW(hwnd,
            L"Mod installed!\n\nEnter the server address (IP:Port from the host) and click 'Play'.",
            L"Install Complete", MB_ICONINFORMATION);
        return TRUE;

    case WM_INSTALL_ERROR: {
        const wchar_t* err = (const wchar_t*)lParam;
        wchar_t msg[512];
        swprintf(msg, L"Error: %s\n\nCheck your internet connection and restart the launcher.", err);
        MessageBoxW(hwnd, msg, L"Error", MB_ICONERROR);
        SetStatus(hwnd, L"Install failed");
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
