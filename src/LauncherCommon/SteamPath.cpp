#include "SteamPath.h"
#include <windows.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <cstdio>
#include <fstream>
#include <sstream>

#pragma comment(lib, "shlwapi.lib")

namespace LauncherCommon {

static std::wstring ReadRegValue(HKEY root, const wchar_t* subkey, const wchar_t* value) {
    HKEY hKey;
    if (RegOpenKeyExW(root, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return L"";

    wchar_t buf[MAX_PATH];
    DWORD bufSize = sizeof(buf);
    DWORD type;
    if (RegQueryValueExW(hKey, value, nullptr, &type, (LPBYTE)buf, &bufSize) != ERROR_SUCCESS ||
        type != REG_SZ) {
        RegCloseKey(hKey);
        return L"";
    }
    RegCloseKey(hKey);
    return std::wstring(buf);
}

// Извлекает значение из строки VDF (кавычки не учитываются)
static std::wstring VDFGetStr(const wchar_t* line, const wchar_t* key) {
    const wchar_t* p = wcsstr(line, key);
    if (!p) return L"";
    p += wcslen(key);
    // Пропуск пробелов
    while (*p == L' ' || *p == L'\t') ++p;
    if (*p != L'"') return L"";
    ++p;
    const wchar_t* end = p;
    while (*end && *end != L'"') ++end;
    return std::wstring(p, end - p);
}

// Ищет app_id 233860 (Kenshi) в libraryfolders.vdf
static std::wstring FindKenshiInLibrary(const std::wstring& steamPath) {
    std::wstring vdfPath = steamPath + L"\\steamapps\\libraryfolders.vdf";
    if (!PathFileExistsW(vdfPath.c_str())) return L"";

    std::ifstream f(vdfPath);
    if (!f) return L"";

    std::wstring curLine;
    std::wstring curPath;
    bool inLibrary = false;

    while (std::getline(f, curLine)) {
        // Удаляем \r в конце строки (Windows)
        if (!curLine.empty() && curLine.back() == L'\r')
            curLine.pop_back();

        if (curLine.find(L'"') != std::wstring::npos) {
            std::wstring token = curLine;
            token.erase(std::remove_if(token.begin(), token.end(),
                [](wchar_t c) { return c == L'\t' || c == L' ' || c == L'\r' || c == L'\n'; }),
                token.end());

            if (VDFGetStr(token.c_str(), L"\"path\"").size() > 0) {
                curPath = VDFGetStr(token.c_str(), L"\"path\"");
                // Заменяем слэши
                for (auto& c : curPath) if (c == L'\\') c = L'/';
            }

            std::wstring apps = VDFGetStr(token.c_str(), L"\"apps\"");
            if (!apps.empty() && apps.find(L"233860") != std::wstring::npos) {
                std::wstring result = curPath + L"/steamapps/common/Kenshi";
                for (auto& c : result) if (c == L'/') c = L'\\';
                return result;
            }
        }
    }

    // Последний library — Steam itself
    std::wstring manifestPath = steamPath + L"\\steamapps\\common\\Kenshi";
    if (PathFileExistsW((manifestPath + L"\\kenshi_x64.exe").c_str()))
        return manifestPath;

    return L"";
}

std::wstring FindKenshiPath() {
    // 1. Пробуем через Steam Registry
    std::wstring steamPath = ReadRegValue(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\WOW6432Node\\Valve\\Steam",
        L"InstallPath"
    );
    if (!steamPath.empty()) {
        std::wstring kenshiPath = FindKenshiInLibrary(steamPath);
        if (!kenshiPath.empty()) return kenshiPath;

        // Резервный путь по умолчанию
        std::wstring def = steamPath + L"\\steamapps\\common\\Kenshi";
        if (PathFileExistsW((def + L"\\kenshi_x64.exe").c_str()))
            return def;
    }

    // 2. 32-bit реестр (fallback)
    std::wstring steamPath32 = ReadRegValue(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Valve\\Steam",
        L"InstallPath"
    );
    if (!steamPath32.empty()) {
        std::wstring kenshiPath = FindKenshiInLibrary(steamPath32);
        if (!kenshiPath.empty()) return kenshiPath;
    }

    return L"";
}

}