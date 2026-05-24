#include "SteamPath.h"
#include <windows.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
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

// Read entire file into string (ASCII/VDF format)
static bool ReadFileToString(const wchar_t* path, std::string& out) {
    FILE* f = _wfopen(path, L"rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    out.resize((size_t)len);
    size_t read = fread(out.data(), 1, (size_t)len, f);
    fclose(f);
    out.resize(read);
    return true;
}

// Parse VDF key: "key" "value" format
static std::string VDFGetStr(const char* line, const char* key) {
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
    std::string content;
    if (!ReadFileToString(std::wstring(vdfPath.begin(), vdfPath.end()).c_str(), content))
        return L"";

    std::string curPath;
    std::string::size_type pos = 0;

    while (pos < content.size()) {
        std::string::size_type lineEnd = content.find('\n', pos);
        if (lineEnd == std::string::npos) lineEnd = content.size();
        std::string line = content.substr(pos, lineEnd - pos);
        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::string pathVal = VDFGetStr(line.c_str(), "\"path\"");
        if (!pathVal.empty()) {
            curPath = pathVal;
            std::replace(curPath.begin(), curPath.end(), '/', '\\');
        }

        std::string appsVal = VDFGetStr(line.c_str(), "\"apps\"");
        if (!appsVal.empty() && appsVal.find("233860") != std::string::npos) {
            std::wstring result = std::wstring(curPath.begin(), curPath.end())
                + L"\\steamapps\\common\\Kenshi";
            FILE* verify = _wfopen((result + L"\\kenshi_x64.exe").c_str(), L"rb");
            if (verify) { fclose(verify); return result; }
        }

        pos = lineEnd + 1;
    }

    // Default path
    std::string defPath = steamPath + "\\steamapps\\common\\Kenshi";
    FILE* verify = _wfopen(std::wstring(defPath.begin(), defPath.end()).c_str(), L"rb");
    if (verify) { fclose(verify); return std::wstring(defPath.begin(), defPath.end()); }

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
        std::wstring kenshi = FindKenshiInLibrary(
            std::string(steamPath.begin(), steamPath.end()));
        if (!kenshi.empty()) return kenshi;
    }

    // Try 32-bit registry
    std::wstring steamPath32 = ReadRegValue(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Valve\\Steam",
        L"InstallPath"
    );
    if (!steamPath32.empty()) {
        std::wstring kenshi = FindKenshiInLibrary(
            std::string(steamPath32.begin(), steamPath32.end()));
        if (!kenshi.empty()) return kenshi;
    }

    // Try user registry
    std::wstring userSteam = ReadRegValue(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Valve\\Steam",
        L"SteamPath"
    );
    if (!userSteam.empty()) {
        std::wstring kenshi = FindKenshiInLibrary(
            std::string(userSteam.begin(), userSteam.end()));
        if (!kenshi.empty()) return kenshi;
    }

    return L"";
}

}