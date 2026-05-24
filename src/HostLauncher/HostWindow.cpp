#include "HostWindow.h"
#include "../LauncherCommon/LauncherCommon.h"
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>
#include <cstring>
#include <memory>

using namespace LauncherCommon;

#pragma comment(lib, "comctl32.lib")

#ifndef Button_SetCheck
#define Button_SetCheck(hwnd, state) ((void)SendMessageW((hwnd), BM_SETCHECK, (WPARAM)(state), 0))
#endif
#ifndef Button_GetCheck
#define Button_GetCheck(hwnd) ((int)(DWORD)(WORD)SendMessageW((hwnd), BM_GETCHECK, 0, 0))
#endif
#ifndef PBM_SETPOS
#define PBM_SETPOS 0x0402
#endif
#ifndef MAKEPARAM
#define MAKEPARAM(a, b) ((LPARAM)MAKELONG(a, b))
#endif

struct ServerConfig {
    std::wstring serverName = L"Kenshi Server";
    int port = 27800;
    int maxPlayers = 5;
    bool pvpEnabled = true;
    bool modInstalled = false;
};

static HWND g_hwnd = nullptr;
static HWND g_hLog = nullptr;
static HWND g_hInstallBtn = nullptr;
static HWND g_hLaunchBtn = nullptr;
static HWND g_hStopBtn = nullptr;
static HWND g_hProgress = nullptr;
static HWND g_hPathCombo = nullptr;
static bool g_serverRunning = false;
static ServerConfig g_config;
static std::wstring g_kenshiPath;
static std::wstring g_localIP;
static int g_localPort = 27800;

static bool VerifyKenshiPath(const std::wstring& path) {
    if (path.empty()) return false;
    std::wstring exe = path + L"\\kenshi_x64.exe";
    return GetFileAttributesW(exe.c_str()) != INVALID_FILE_ATTRIBUTES;
}

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
    EnableWindow(GetDlgItem(hwnd, 102), enable);
    EnableWindow(GetDlgItem(hwnd, 103), enable);
    EnableWindow(GetDlgItem(hwnd, 104), enable);
    EnableWindow(GetDlgItem(hwnd, 105), enable);
}

static void UpdateModStatus(HWND hwnd, const std::wstring& path) {
    bool installed = IsModInstalled(path);
    g_config.modInstalled = installed;
    if (installed) {
        SetDlgItemTextW(hwnd, 2, L"[OK] Kenshi-Online mod installed");
    } else {
        SetDlgItemTextW(hwnd, 2, L"[--] Kenshi-Online mod NOT installed");
    }
    EnableWindow(g_hLaunchBtn, installed ? TRUE : FALSE);
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

static void RefreshIP(HWND hwnd) {
    g_localIP = GetLocalVPNIP();
    if (g_localIP.empty()) g_localIP = GetLocalIP();
    wchar_t buf[128];
    swprintf(buf, L"Your IP (VPN): %s", g_localIP.c_str());
    SetDlgItemTextW(hwnd, 300, buf);
}

static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG: {
        g_hwnd = hwnd;
        SetWindowPos(hwnd, nullptr, 0, 0, 500, 620, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowTextW(hwnd, L"Kenshi - Host Launcher");

        g_hPathCombo = GetDlgItem(hwnd, 1);
        g_hInstallBtn = GetDlgItem(hwnd, 3);
        g_hLaunchBtn = GetDlgItem(hwnd, 4);
        g_hStopBtn = GetDlgItem(hwnd, 5);
        g_hProgress = GetDlgItem(hwnd, 101);

        SendMessageW(g_hProgress, PBM_SETRANGE, 0, MAKEPARAM(0, 100));
        SendMessageW(g_hProgress, PBM_SETPOS, 0, 0);

        // Auto-detect Kenshi
        std::wstring detected = FindKenshiPath();
        if (!detected.empty()) {
            SetKenshiPath(hwnd, detected);
        }

        RefreshIP(hwnd);

        // Default server settings
        SetDlgItemTextW(hwnd, 103, g_config.serverName.c_str());
        wchar_t portBuf[16];
        swprintf(portBuf, L"%d", g_config.port);
        SetDlgItemTextW(hwnd, 102, portBuf);
        SendMessageW(GetDlgItem(hwnd, 104), TBM_SETRANGE, TRUE, MAKELONG(1, 5));
        SendMessageW(GetDlgItem(hwnd, 104), TBM_SETPOS, TRUE, g_config.maxPlayers);
        wchar_t playersBuf[32];
        swprintf(playersBuf, L"Max Players: %d", g_config.maxPlayers);
        SetDlgItemTextW(hwnd, 106, playersBuf);
        Button_SetCheck(GetDlgItem(hwnd, 105), g_config.pvpEnabled ? BST_CHECKED : BST_UNCHECKED);

        EnableWindow(g_hLaunchBtn, g_config.modInstalled ? TRUE : FALSE);
        EnableWindow(g_hStopBtn, FALSE);

        AddLog(hwnd, L"=== Kenshi Network Launcher ===");
        AddLog(hwnd, L"1. Select Kenshi folder (Browse) or it will auto-detect");
        AddLog(hwnd, L"2. Click 'Install Mod' to download Kenshi-Online");
        AddLog(hwnd, L"3. Click 'Start Server'");
        AddLog(hwnd, L"4. Copy IP and share with friends");
        return TRUE;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);

        if (id == 10) { // Browse button
            wchar_t selectedPath[MAX_PATH] = {};
            BROWSEINFOW bi = {};
            bi.lpszTitle = L"Select Kenshi folder";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            bi.hwndOwner = hwnd;
            LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
            if (pidl) {
                if (SHGetPathFromIDListW(pidl, selectedPath)) {
                    SetKenshiPath(hwnd, selectedPath);
                    AddLog(hwnd, L"Path updated.");
                }
                IMalloc* imalloc = nullptr;
                if (SUCCEEDED(SHGetMalloc(&imalloc))) {
                    imalloc->Free(pidl);
                    imalloc->Release();
                }
            }
            return TRUE;
        }

        if (id == 1 && HIWORD(wParam) == CBN_EDITCHANGE) {
            // User typed in combobox
            wchar_t buf[MAX_PATH];
            GetWindowTextW(g_hPathCombo, buf, MAX_PATH);
            std::wstring path(buf);
            if (VerifyKenshiPath(path)) {
                SetKenshiPath(hwnd, path);
            }
            return TRUE;
        }

        if (id == 3) { // Install Mod
            wchar_t buf[MAX_PATH];
            GetWindowTextW(g_hPathCombo, buf, MAX_PATH);
            g_kenshiPath = buf;

            if (!VerifyKenshiPath(g_kenshiPath)) {
                MessageBoxW(hwnd, L"Please select a valid Kenshi folder (the folder containing kenshi_x64.exe)", L"Error", MB_ICONERROR);
                return TRUE;
            }

            EnableWindow(g_hInstallBtn, FALSE);
            SetStatus(hwnd, L"Downloading Kenshi-Online...");
            SetProgress(hwnd, 10);
            AddLog(hwnd, L"Connecting to GitHub...");

            std::thread([hwnd]() {
                LatestRelease release;
                if (!FetchLatestRelease("The404Studios", "Kenshi-Online", release)) {
                    PostMessageW(hwnd, WM_USER + 1, 2, (LPARAM)L"Error: Could not fetch release info. Check internet connection.");
                    return;
                }

                wchar_t logBuf[256];
                swprintf(logBuf, L"Found release: %S", release.version.c_str());
                AddLog(hwnd, logBuf);
                SetProgress(hwnd, 30);

                AddLog(hwnd, L"Downloading archive...");
                std::wstring modPath = DownloadAndExtract(release.zipUrl, g_kenshiPath);
                SetProgress(hwnd, 80);

                if (modPath.empty()) {
                    PostMessageW(hwnd, WM_USER + 1, 2, (LPARAM)L"Error: Download failed. Check internet connection.");
                    return;
                }

                AddLog(hwnd, L"Installing mod files...");
                std::wstring err;
                DeployResult res = DeployModFiles(g_kenshiPath, modPath, &err);
                SetProgress(hwnd, 100);

                if (res == DeployResult::Success) {
                    PostMessageW(hwnd, WM_USER + 1, 3, 0);
                } else if (res == DeployResult::KenshiNotFound) {
                    PostMessageW(hwnd, WM_USER + 1, 2, (LPARAM)L"Error: Kenshi folder not valid.");
                } else {
                    wchar_t errBuf[512];
                    swprintf(errBuf, L"Error: %d", (int)res);
                    PostMessageW(hwnd, WM_USER + 1, 2, (LPARAM)errBuf);
                }
            }).detach();
            return TRUE;
        }

        if (id == 4) { // Start Server
            if (!g_config.modInstalled) {
                MessageBoxW(hwnd, L"Please install the mod first.", L"Error", MB_ICONERROR);
                return TRUE;
            }

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
            SetStatus(hwnd, L"Starting server...");

            AddLog(hwnd, L"Generating server.json...");
            AddLog(hwnd, L"Starting KenshiMP.Server.exe...");

            std::thread([hwnd, port, players, pvp]() {
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
                    AddLog(hwnd, L"server.json created.");
                } else {
                    AddLog(hwnd, L"Error: Could not create server.json");
                    PostMessageW(hwnd, WM_USER + 1, 4, 0);
                    return;
                }

                std::wstring serverExe = g_kenshiPath + L"\\KenshiMP.Server.exe";
                if (GetFileAttributesW(serverExe.c_str()) == INVALID_FILE_ATTRIBUTES) {
                    AddLog(hwnd, L"Error: KenshiMP.Server.exe not found. Reinstall the mod.");
                    PostMessageW(hwnd, WM_USER + 1, 2, (LPARAM)L"Error: KenshiMP.Server.exe not found. Reinstall the mod.");
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

                BOOL created = CreateProcessW(serverExe.c_str(), nullptr,
                    nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                    g_kenshiPath.c_str(), &si, &pi);

                if (!created) {
                    AddLog(hwnd, L"Error: Could not start server");
                    PostMessageW(hwnd, WM_USER + 1, 2, (LPARAM)L"Error: Could not start KenshiMP.Server.exe");
                    PostMessageW(hwnd, WM_USER + 1, 4, 0);
                    CloseHandle(hOutR);
                    CloseHandle(hOutW);
                    return;
                }

                CloseHandle(hOutW);
                g_serverRunning = true;

                char lineBuf[4096];
                DWORD bytesRead;
                while (g_serverRunning) {
                    if (!ReadFile(hOutR, lineBuf, sizeof(lineBuf) - 1, &bytesRead, nullptr) || bytesRead == 0)
                        break;

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

                    DWORD code;
                    if (!GetExitCodeProcess(pi.hProcess, &code) || code != STILL_ACTIVE)
                        break;
                }

                AddLog(hwnd, L"Server stopped.");
                PostMessageW(hwnd, WM_USER + 1, 4, 0);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                CloseHandle(hOutR);
            }).detach();

            return TRUE;
        }

        if (id == 5) { // Stop
            g_serverRunning = false;
            AddLog(hwnd, L"Stopping server...");
            EnableWindow(g_hStopBtn, FALSE);
            SetStatus(hwnd, L"Server stopping...");
            return TRUE;
        }

        if (id == 104 + 1000) { // Slider scroll
            int pos = SendMessageW((HWND)lParam, TBM_GETPOS, 0, 0);
            wchar_t buf[64];
            swprintf(buf, L"Max Players: %d", pos);
            SetDlgItemTextW(hwnd, 106, buf);
            return TRUE;
        }

        if (id == 7) { // Copy IP
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
            AddLog(hwnd, L"IP copied to clipboard!");
            return TRUE;
        }

        if (id == 6) { // Exit
            g_serverRunning = false;
            EndDialog(hwnd, 0);
            return TRUE;
        }

        return FALSE;
    }

    case WM_USER + 1: {
        int type = (int)wParam;
        const wchar_t* msg = (const wchar_t*)lParam;

        switch (type) {
        case 2: // Error
            MessageBoxW(hwnd, msg, L"Error", MB_ICONERROR);
            EnableWindow(g_hInstallBtn, TRUE);
            SetProgress(hwnd, 0);
            SetStatus(hwnd, L"Error");
            break;
        case 3: // Install complete
            g_config.modInstalled = true;
            SetDlgItemTextW(hwnd, 2, L"[OK] Kenshi-Online mod installed!");
            SetStatus(hwnd, L"Mod installed!");
            SetProgress(hwnd, 100);
            EnableWindow(g_hInstallBtn, FALSE);
            EnableWindow(g_hLaunchBtn, TRUE);
            AddLog(hwnd, L"=== Install complete ===");
            AddLog(hwnd, L"Click 'Start Server' to begin.");
            break;
        case 4: // Server stopped
            EnableControls(hwnd, true);
            EnableWindow(g_hInstallBtn, TRUE);
            EnableWindow(g_hLaunchBtn, TRUE);
            EnableWindow(g_hStopBtn, FALSE);
            SetStatus(hwnd, L"Server stopped");
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
    swprintf(buf, L"Your IP (VPN): %s:%d", ip, port);
    SetDlgItemTextW(hwnd, 300, buf);
}
void HostWindowNotifyInstallComplete(HWND hwnd) { PostMessageW(hwnd, WM_USER + 1, 3, 0); }
void HostWindowNotifyError(HWND hwnd, const wchar_t* err) { PostMessageW(hwnd, WM_USER + 1, 2, (LPARAM)err); }
