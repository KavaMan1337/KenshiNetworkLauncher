#include "SteamPath.h"
#include <windows.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

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

// Parse VDF key: "key" "value" format
static std::string VDFGetStrA(const char* line, const char* key) {
    const char* p = strstr(line, key);
    if (!p) return "";
    p += strlen(key);
    while (*p == ' ' || *p == '\t') ++p;
    if (*p != '"') return "";
    ++p;
    const char* end = p;
    while (*end && *end != '"') ++end;
    return std::string(p, end - p);
}

static std::wstring FindKenshiInLibrary(const std::string& steamPath) {
    std::string vdfPath = steamPath + "\\steamapps\\libraryfolders.vdf";
    FILE* f = _wfopen(std::wstring(vdfPath.begin(), vdfPath.end()).c_str(), L"r, ccs=UTF-8");
    if (!f) return L"";

    std::string curLine;
    std::string curPath;
    bool found = false;

    while (std::getline(f, curLine)) {
        if (!curLine.empty() && curLine.back() == '\r')
            curLine.pop_back();

        std::string pathVal = VDFGetStrA(curLine.c_str(), "\"path\"");
        if (!pathVal.empty()) {
            curPath = pathVal;
            std::replace(curPath.begin(), curPath.end(), '/', '\\');
        }

        std::string appsVal = VDFGetStrA(curLine.c_str(), "\"apps\"");
        if (!appsVal.empty() && appsVal.find("233860") != std::string::npos) {
            std::wstring result = std::wstring(curPath.begin(), curPath.end()) + L"\\steamapps\\common\\Kenshi";
            fclose(f);

            // Verify kenshi_x64.exe exists
            FILE* verify = _wfopen((result + L"\\kenshi_x64.exe").c_str(), L"rb");
            if (verify) { fclose(verify); return result; }
            return L"";
        }
    }
    fclose(f);

    // Check default path
    std::string defPath = steamPath + "\\steamapps\\common\\Kenshi";
    FILE* verify = _wfopen(std::wstring(defPath.begin(), defPath.end()).c_str(), L"rb");
    if (verify) {
        fclose(verify);
        return std::wstring(defPath.begin(), defPath.end());
    }

    return L"";
}

std::wstring FindKenshiPath() {
    // Try Steam Registry (64-bit)
    std::wstring steamPath = ReadRegValue(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\WOW6432Node\\Valve\\Steam",
        L"InstallPath"
    );
    if (!steamPath.empty()) {
        std::wstring kenshi = FindKenshiInLibrary(std::string(steamPath.begin(), steamPath.end()));
        if (!kenshi.empty()) return kenshi;

        // Default path fallback
        std::wstring def = steamPath + L"\\steamapps\\common\\Kenshi";
        FILE* verify = _wfopen((def + L"\\kenshi_x64.exe").c_str(), L"rb");
        if (verify) { fclose(verify); return def; }
    }

    // Try 32-bit registry
    std::wstring steamPath32 = ReadRegValue(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Valve\\Steam",
        L"InstallPath"
    );
    if (!steamPath32.empty()) {
        std::wstring kenshi = FindKenshiInLibrary(std::string(steamPath32.begin(), steamPath32.end()));
        if (!kenshi.empty()) return kenshi;
    }

    // Try user registry (HKEY_CURRENT_USER has lower priority)
    std::wstring userSteam = ReadRegValue(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Valve\\Steam",
        L"SteamPath"
    );
    if (!userSteam.empty()) {
        std::wstring kenshi = FindKenshiInLibrary(std::string(userSteam.begin(), userSteam.end()));
        if (!kenshi.empty()) return kenshi;
    }

    return L"";
}

}