#include "GitHubReleases.h"
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <fstream>
#include <shlobj.h>
#include <KnownFolders.h>

#pragma comment(lib, "winhttp.lib")

namespace {

// Простой JSON-парсер (без внешних зависимостей)
// Ищет "key": "value" или "key": [array]
std::string JsonStr(const char* json, const char* key) {
    std::string needle = std::string("\"") + key + "\":\"";
    const char* p = strstr(json, needle.c_str());
    if (!p) return "";
    p += needle.size();
    const char* end = p;
    while (*end && *end != '"' && *end != '\n') ++end;
    return std::string(p, end - p);
}

// Поиск объекта по ключу
const char* JsonObj(const char* json, const char* key) {
    std::string needle = std::string("\"") + key + "\":{";
    const char* p = strstr(json, needle.c_str());
    if (!p) return nullptr;
    return p + needle.size() - 1; // на '{'
}

bool WriteFile(const std::wstring& path, const void* data, size_t size) {
    FILE* f = _wfopen(path.c_str(), L"wb");
    if (!f) return false;
    fwrite(data, 1, size, f);
    fclose(f);
    return true;
}

}

namespace LauncherCommon {

static std::wstring GetTempPath() {
    wchar_t buf[MAX_PATH];
    DWORD ret = GetTempPathW(MAX_PATH, buf);
    if (ret == 0) return L"C:\\Temp";
    return std::wstring(buf);
}

static std::wstring BuildApiUrl(const char* owner, const char* repo) {
    return std::wstring(L"https://api.github.com/repos/") +
           std::wstring(owner, owner + strlen(owner)) + L"/" +
           std::wstring(repo, repo + strlen(repo)) + L"/releases/latest";
}

bool FetchLatestRelease(const char* owner, const char* repo, LatestRelease& out) {
    std::wstring url = BuildApiUrl(owner, repo);

    HINTERNET hSession = WinHttpOpen(L"KenshiLauncher/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
    if (!hSession) return false;

    bool ok = false;
    HINTERNET hConnect = WinHttpOpenRequest(hSession, L"GET", url.c_str(),
        nullptr, nullptr, nullptr, 0);
    if (hConnect) {
        WinHttpSetOption(hConnect, WINHTTP_OPTION_USER_AGENT, L"KenshiLauncher/1.0", 16);
        if (WinHttpSendRequest(hConnect, nullptr, 0, nullptr, 0, 0, 0)) {
            DWORD statusCode = 0, statusLen = sizeof(statusCode);
            WinHttpQueryHeaders(hConnect, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                nullptr, &statusCode, &statusLen, nullptr);
            if (statusCode == 200) {
                std::string response;
                char buf[4096];
                DWORD bytesRead;
                while (WinHttpReadData(hConnect, buf, sizeof(buf), &bytesRead) && bytesRead > 0) {
                    response.append(buf, bytesRead);
                }
                if (!response.empty()) {
                    out.tagName = JsonStr(response.c_str(), "tag_name");
                    out.version = out.tagName.empty() ? "latest" : out.tagName;

                    // Ищем assets и ищем zip
                    const char* assetsPtr = strstr(response.c_str(), "\"assets\":[");
                    if (assetsPtr) {
                        const char* nextBracket = strstr(assetsPtr + 9, "[");
                        const char* endBracket = nullptr;
                        if (nextBracket) {
                            int depth = 1;
                            const char* q = nextBracket + 1;
                            while (*q && depth > 0) {
                                if (*q == '[') ++depth;
                                else if (*q == ']') --depth;
                                ++q;
                            }
                            endBracket = q;
                        }

                        std::string assetsJson;
                        if (nextBracket && endBracket && nextBracket < endBracket)
                            assetsJson = std::string(nextBracket, endBracket - nextBracket + 1);

                        // Ищем browser_download_url
                        const char* urlPtr = strstr(assetsJson.c_str(), "\"browser_download_url\"");
                        if (urlPtr) {
                            const char* colon = strstr(urlPtr, "\":\"");
                            if (colon) {
                                const char* start = colon + 3;
                                const char* endUrl = strstr(start, "\"");
                                if (endUrl) {
                                    out.zipUrl = std::string(start, endUrl - start);
                                    ok = true;
                                }
                            }
                        }
                    }
                }
            }
        }
        WinHttpCloseHandle(hConnect);
    }
    WinHttpCloseHandle(hSession);
    return ok;
}

bool DownloadFile(const std::string& url, const std::wstring& destPath) {
    // Парсим URL
    std::wstring wUrl(url.begin(), url.end());

    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = {}, path[2048] = {};
    uc.lpszHostName = host;
    uc.lpszUrlPath = path;
    uc.dwHostNameLength = 256;
    uc.dwUrlPathLength = 2048;
    if (!WinHttpCrackUrl(wUrl.c_str(), (DWORD)wUrl.size(), 0, &uc))
        return false;

    HINTERNET hSession = WinHttpOpen(L"KenshiLauncher/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
    if (!hSession) return false;

    bool ok = false;
    HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
    if (hConnect) {
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
            nullptr, nullptr, nullptr, (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0);
        if (hRequest) {
            if (WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0)) {
                DWORD statusCode = 0, statusLen = sizeof(statusCode);
                WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                    nullptr, &statusCode, &statusLen, nullptr);
                if (statusCode == 200 || statusCode == 302 || statusCode == 301) {
                    std::vector<char> data;
                    char buf[16384];
                    DWORD bytesRead;

                    // Handle redirects manually
                    if (statusCode == 302 || statusCode == 301) {
                        DWORD locLen = 0;
                        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_LOCATION, nullptr, nullptr, &locLen, nullptr);
                        if (locLen > 0) {
                            std::wstring redirectUrl(locLen, L'\0');
                            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_LOCATION, nullptr,
                                &redirectUrl[0], &locLen, nullptr);
                            WinHttpCloseHandle(hRequest);
                            WinHttpCloseHandle(hConnect);
                            WinHttpCloseHandle(hSession);
                            return DownloadFile(std::string(redirectUrl.begin(), redirectUrl.end()), destPath);
                        }
                    }

                    while (WinHttpReadData(hRequest, buf, sizeof(buf), &bytesRead) && bytesRead > 0) {
                        data.insert(data.end(), buf, buf + bytesRead);
                    }
                    if (!data.empty()) {
                        ok = WriteFile(destPath, data.data(), data.size());
                    }
                }
            }
            WinHttpCloseHandle(hRequest);
        }
        WinHttpCloseHandle(hConnect);
    }
    WinHttpCloseHandle(hSession);
    return ok;
}

// Извлекает ZIP в папку (простая реализация без внешних библиотек)
// Использует built-in Windows shell
static bool ExtractZip(const std::wstring& zipPath, const std::wstring& destDir) {
    // Создаём папку назначения
    CreateDirectoryW(destDir.c_str(), nullptr);

    // Используем IShellDispatch и Folder::CopyHere
    // Для простоты используем PowerShell
    std::wstring psCmd = L"Expand-Archive -Path \"" + zipPath + L"\" -DestinationPath \"" + destDir + L"\" -Force";
    std::string cmdA(psCmd.begin(), psCmd.end());
    cmdA = "powershell -NoProfile -Command \"" + cmdA + "\"";

    int ret = system(cmdA.c_str());
    return ret == 0;
}

std::wstring DownloadAndExtract(const std::string& url, const std::wstring& destDir) {
    std::wstring tmpDir = GetTempPath() + L"\\KenshiLauncher";
    CreateDirectoryW(tmpDir.c_str(), nullptr);
    std::wstring zipPath = tmpDir + L"\\release.zip";

    if (!DownloadFile(url, zipPath)) {
        DeleteFileW(zipPath.c_str());
        return L"";
    }

    std::wstring extractDir = tmpDir + L"\\extracted";
    CreateDirectoryW(extractDir.c_str(), nullptr);

    if (!ExtractZip(zipPath, extractDir)) {
        DeleteFileW(zipPath.c_str());
        RemoveDirectoryW(extractDir.c_str());
        return L"";
    }

    DeleteFileW(zipPath.c_str());

    // Ищем папку dist внутри распакованного архива
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((extractDir + L"\\*").c_str(), &fd);
    std::wstring distDir;
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                std::wstring name(fd.cFileName);
                if (name != L"." && name != L".." && name != L"dist") {
                    // Папка верхнего уровня — ищем dist внутри
                    std::wstring inner = extractDir + L"\\" + name + L"\\dist";
                    if (GetFileAttributesW(inner.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        distDir = inner;
                        break;
                    }
                    // Или сама папка — проверяем есть ли там наши файлы
                    std::wstring coreDll = extractDir + L"\\" + name + L"\\KenshiMP.Core.dll";
                    if (GetFileAttributesW(coreDll.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        distDir = extractDir + L"\\" + name;
                        break;
                    }
                }
            }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }

    // Если не нашли dist, используем корень
    if (distDir.empty()) distDir = extractDir;

    // Копируем файлы из dist в destDir
    h = FindFirstFileW((distDir + L"\\*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            std::wstring src = distDir + L"\\" + fd.cFileName;
            std::wstring dst = destDir + L"\\" + fd.cFileName;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0) {
                    CreateDirectoryW(dst.c_str(), nullptr);
                    // Рекурсивно копируем подпапки (например data/)
                    WIN32_FIND_DATAW sfd;
                    HANDLE sh = FindFirstFileW((src + L"\\*").c_str(), &sfd);
                    if (sh != INVALID_HANDLE_VALUE) {
                        do {
                            if (sfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                                if (wcscmp(sfd.cFileName, L".") != 0 && wcscmp(sfd.cFileName, L"..") != 0) {
                                    std::wstring sdst = dst + L"\\" + sfd.cFileName;
                                    CreateDirectoryW(sdst.c_str(), nullptr);
                                    // Копируем содержимое
                                    WIN32_FIND_DATAW ffd;
                                    HANDLE fh = FindFirstFileW((src + L"\\" + sfd.cFileName + L"\\*").c_str(), &ffd);
                                    if (fh != INVALID_HANDLE_VALUE) {
                                        do {
                                            std::wstring ssrc = src + L"\\" + sfd.cFileName + L"\\" + ffd.cFileName;
                                            std::wstring ddst = sdst + L"\\" + ffd.cFileName;
                                            CopyFileW(ssrc.c_str(), ddst.c_str(), FALSE);
                                        } while (FindNextFileW(fh, &ffd));
                                        FindClose(fh);
                                    }
                                }
                            } else {
                                std::wstring ssrc = src + L"\\" + sfd.cFileName;
                                std::wstring ddst = dst + L"\\" + sfd.cFileName;
                                CopyFileW(ssrc.c_str(), ddst.c_str(), FALSE);
                            }
                        } while (FindNextFileW(sh, &sfd));
                        FindClose(sh);
                    }
                }
            } else {
                CopyFileW(src.c_str(), dst.c_str(), FALSE);
            }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }

    // Удаляем временную папку
    RemoveDirectoryW(extractDir.c_str());

    return destDir;
}

}