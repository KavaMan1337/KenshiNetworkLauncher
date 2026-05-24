#include "GitHubReleases.h"
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <fstream>

#pragma comment(lib, "winhttp.lib")

namespace {

// Simple JSON parser - finds "key": "value"
static std::string JsonStr(const char* json, const char* key) {
    std::string needle = std::string("\"") + key + "\":\"";
    const char* p = strstr(json, needle.c_str());
    if (!p) return "";
    p += needle.size();
    const char* end = p;
    while (*end && *end != '"' && *end != '\n') ++end;
    return std::string(p, end - p);
}

static bool WriteFile(const wchar_t* path, const void* data, size_t size) {
    FILE* f = _wfopen(path, L"wb");
    if (!f) return false;
    fwrite(data, 1, size, f);
    fclose(f);
    return true;
}

static std::wstring GetTmpDir() {
    wchar_t buf[MAX_PATH];
    DWORD ret = GetTempPathW(MAX_PATH, buf);
    if (ret == 0) return L"C:\\Temp";
    std::wstring s(buf);
    if (!s.empty() && s.back() == L'\\') s.pop_back();
    return s + L"\\KenshiLauncher";
}

static std::wstring WideUrl(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

static std::string NarrowUrl(const wchar_t* s) {
    return std::string(s, s + wcslen(s));
}

} // anonymous namespace

namespace LauncherCommon {

bool FetchLatestRelease(const char* owner, const char* repo, LatestRelease& out) {
    std::wstring apiUrl = L"https://api.github.com/repos/" +
        WideUrl(owner) + L"/" + WideUrl(repo) + L"/releases/latest";

    HINTERNET hSession = WinHttpOpen(L"KenshiLauncher/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
    if (!hSession) return false;

    bool ok = false;
    HINTERNET hConnect = WinHttpOpenRequest(hSession, L"GET", apiUrl.c_str(),
        nullptr, nullptr, nullptr, 0);
    if (hConnect) {
        WinHttpSetOption(hConnect, WINHTTP_OPTION_USER_AGENT, L"KenshiLauncher/1.0", 16);
        if (WinHttpSendRequest(hConnect, nullptr, 0, nullptr, 0, 0, 0)) {
            DWORD statusCode = 0, statusLen = sizeof(statusCode);
            WinHttpQueryHeaders(hConnect,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                nullptr, &statusCode, &statusLen, nullptr);
            if (statusCode == 200) {
                std::string response;
                char buf[8192];
                DWORD bytesRead;
                while (WinHttpReadData(hConnect, buf, sizeof(buf), &bytesRead) && bytesRead > 0) {
                    response.append(buf, bytesRead);
                }
                if (!response.empty()) {
                    out.tagName = JsonStr(response.c_str(), "tag_name");
                    out.version = out.tagName.empty() ? "latest" : out.tagName;

                    // Find assets array
                    const char* assetsPtr = strstr(response.c_str(), "\"assets\":[");
                    if (assetsPtr) {
                        const char* openBr = strstr(assetsPtr + 9, "[");
                        if (openBr) {
                            int depth = 1;
                            const char* q = openBr + 1;
                            while (*q && depth > 0) {
                                if (*q == '[') ++depth;
                                else if (*q == ']') --depth;
                                ++q;
                            }
                            if (openBr < q && depth == 0) {
                                std::string assetsJson(openBr, q - openBr + 1);
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
            }
        }
        WinHttpCloseHandle(hConnect);
    }
    WinHttpCloseHandle(hSession);
    return ok;
}

bool DownloadFile(const std::string& url, const wchar_t* destPath) {
    std::wstring wUrl = WideUrl(url);
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
        DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
            nullptr, nullptr, nullptr, flags);
        if (hRequest) {
            if (WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0)) {
                DWORD statusCode = 0, statusLen = sizeof(statusCode);
                WinHttpQueryHeaders(hRequest,
                    WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                    nullptr, &statusCode, &statusLen, nullptr);

                if (statusCode == 200 || statusCode == 302 || statusCode == 301) {
                    if (statusCode == 302 || statusCode == 301) {
                        WinHttpCloseHandle(hRequest);
                        WinHttpCloseHandle(hConnect);
                        WinHttpCloseHandle(hSession);
                        return DownloadFile(url, destPath);
                    }

                    std::vector<char> data;
                    char buf[16384];
                    DWORD bytesRead;
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

static bool ExtractZip(const wchar_t* zipPath, const wchar_t* destDir) {
    std::string cmd = "powershell -NoProfile -Command \"Expand-Archive -Path '";
    cmd += NarrowUrl(zipPath) + "' -DestinationPath '" + NarrowUrl(destDir) + "' -Force\"";
    return system(cmd.c_str()) == 0;
}

std::wstring DownloadAndExtract(const std::string& url, const wchar_t* destDir) {
    std::wstring tmpDir = GetTmpDir();
    CreateDirectoryW(tmpDir.c_str(), nullptr);
    std::wstring zipPath = tmpDir + L"\\release.zip";

    if (!DownloadFile(url, zipPath.c_str())) {
        DeleteFileW(zipPath.c_str());
        return L"";
    }

    std::wstring extractDir = tmpDir + L"\\extracted";
    CreateDirectoryW(extractDir.c_str(), nullptr);

    if (!ExtractZip(zipPath.c_str(), extractDir.c_str())) {
        DeleteFileW(zipPath.c_str());
        RemoveDirectoryW(extractDir.c_str());
        return L"";
    }

    DeleteFileW(zipPath.c_str());

    // Find dist folder or root
    std::wstring distDir;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((extractDir + L"\\*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                std::wstring name(fd.cFileName);
                if (name != L"." && name != L".." && name != L"dist") {
                    std::wstring inner = extractDir + L"\\" + name;
                    std::wstring coreDll = inner + L"\\KenshiMP.Core.dll";
                    if (GetFileAttributesW(coreDll.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        distDir = inner;
                        break;
                    }
                }
            }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    if (distDir.empty()) distDir = extractDir;

    // Copy files
    h = FindFirstFileW((distDir + L"\\*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            std::wstring src = distDir + L"\\" + fd.cFileName;
            std::wstring dst = std::wstring(destDir) + L"\\" + fd.cFileName;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0) {
                    CreateDirectoryW(dst.c_str(), nullptr);
                    WIN32_FIND_DATAW sfd;
                    HANDLE sh = FindFirstFileW((src + L"\\*").c_str(), &sfd);
                    if (sh != INVALID_HANDLE_VALUE) {
                        do {
                            if (sfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                                if (wcscmp(sfd.cFileName, L".") != 0 && wcscmp(sfd.cFileName, L"..") != 0) {
                                    std::wstring sdst = dst + L"\\" + sfd.cFileName;
                                    CreateDirectoryW(sdst.c_str(), nullptr);
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

    RemoveDirectoryW(extractDir.c_str());
    return std::wstring(destDir);
}

} // namespace LauncherCommon