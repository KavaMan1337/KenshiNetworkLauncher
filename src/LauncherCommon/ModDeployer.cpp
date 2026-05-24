#include "ModDeployer.h"
#include <windows.h>
#include <shlwapi.h>
#include <fstream>
#include <string>
#include <algorithm>

#pragma comment(lib, "shlwapi.lib")

namespace LauncherCommon {

static std::wstring ToLower(const std::wstring& s) {
    std::wstring r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::towlower);
    return r;
}

bool IsModInstalled(const std::wstring& kenshiPath) {
    if (kenshiPath.empty()) return false;
    std::wstring dll = kenshiPath + L"\\KenshiMP.Core.dll";
    return GetFileAttributesW(dll.c_str()) != INVALID_FILE_ATTRIBUTES;
}

static bool EnsurePluginsLine(const std::wstring& line, std::vector<std::wstring>& out) {
    // Стандартный поиск без учёта регистра
    std::wstring lowerLine = ToLower(line);
    if (lowerLine.find(L"plugin") != std::wstring::npos ||
        lowerLine.find(L"kenshimp") != std::wstring::npos) {
        // Уже есть — не добавляем
        return false;
    }
    return true;
}

bool PatchPluginsConfig(const std::wstring& kenshiPath) {
    std::wstring cfgPath = kenshiPath + L"\\data\\config\\Plugins_x64.cfg";
    if (GetFileAttributesW(cfgPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        // Создаём файл
        std::ofstream f(cfgPath, std::ios::binary);
        if (!f) return false;
        f << "Plugin=KenshiMP.Core\n";
        return true;
    }

    // Читаем существующий файл
    std::ifstream f(cfgPath, std::ios::binary);
    if (!f) return false;

    std::string content;
    f.seekg(0, std::ios::end);
    content.resize(f.tellg());
    f.seekg(0, std::ios::beg);
    f.read(content.data(), content.size());
    f.close();

    std::string lowerContent = content;
    std::transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);
    if (lowerContent.find("kenshimp.core") != std::string::npos ||
        lowerContent.find("plugin=kenshimp") != std::string::npos) {
        // Уже пропатчено
        return true;
    }

    // Добавляем строку
    std::ofstream out(cfgPath, std::ios::binary | std::ios::app);
    if (!out) return false;
    out << "\nPlugin=KenshiMP.Core\n";
    return true;
}

// Рекурсивное копирование папки
static bool CopyDirectoryTree(const std::wstring& src, const std::wstring& dst) {
    CreateDirectoryW(dst.c_str(), nullptr);

    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((src + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return false;

    bool ok = true;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
            continue;

        std::wstring s = src + L"\\" + fd.cFileName;
        std::wstring d = dst + L"\\" + fd.cFileName;

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!CopyDirectoryTree(s, d)) ok = false;
        } else {
            if (!CopyFileW(s.c_str(), d.c_str(), FALSE))
                ok = false;
        }
    } while (FindNextFileW(h, &fd));

    FindClose(h);
    return ok;
}

// Копирует layout файлы в data/gui/layout/
static bool DeployLayoutFiles(const std::wstring& modFilesPath, const std::wstring& kenshiPath) {
    std::wstring layoutDir = kenshiPath + L"\\data\\gui\\layout\\";
    CreateDirectoryW(layoutDir.c_str(), nullptr);

    const wchar_t* layouts[] = {
        L"Kenshi_MainMenu.layout",
        L"Kenshi_MultiplayerPanel.layout",
        L"Kenshi_MultiplayerHUD.layout"
    };

    for (auto& name : layouts) {
        std::wstring src = modFilesPath + L"\\" + name;
        std::wstring dst = layoutDir + name;
        if (GetFileAttributesW(src.c_str()) != INVALID_FILE_ATTRIBUTES) {
            CopyFileW(src.c_str(), dst.c_str(), FALSE);
        }
    }
    return true;
}

DeployResult DeployModFiles(const std::wstring& kenshiPath, const std::wstring& modFilesPath,
    std::wstring* outError) {
    if (kenshiPath.empty()) return DeployResult::KenshiNotFound;
    if (modFilesPath.empty()) return DeployResult::DownloadFailed;

    std::wstring exePath = kenshiPath + L"\\kenshi_x64.exe";
    if (GetFileAttributesW(exePath.c_str()) == INVALID_FILE_ATTRIBUTES)
        return DeployResult::KenshiNotFound;

    // Основные файлы
    struct FileEntry { const wchar_t* name; bool required; }
    files[] = {
        {L"KenshiMP.Core.dll",      true},
        {L"KenshiMP.Server.exe",    true},
        {L"kenshi-online.mod",      true},
    };

    for (auto& e : files) {
        std::wstring src = modFilesPath + L"\\" + e.name;
        std::wstring dst = kenshiPath + L"\\" + e.name;
        if (GetFileAttributesW(src.c_str()) != INVALID_FILE_ATTRIBUTES) {
            if (!CopyFileW(src.c_str(), dst.c_str(), FALSE)) {
                if (outError) *outError = std::wstring(L"Не удалось скопировать ") + e.name;
                return DeployResult::FileCopyFailed;
            }
        } else if (e.required) {
            if (outError) *outError = std::wstring(L"Файл не найден: ") + e.name;
            return DeployResult::FileCopyFailed;
        }
    }

    // Layout файлы
    DeployLayoutFiles(modFilesPath, kenshiPath);

    // Патчим конфиг плагинов
    if (!PatchPluginsConfig(kenshiPath)) {
        return DeployResult::ConfigPatchFailed;
    }

    return DeployResult::Success;
}

}