#pragma once
#include <windows.h>

namespace LauncherCommon {
    bool IsModInstalled(const std::wstring& kenshiPath);
    DeployResult DeployModFiles(const std::wstring& kenshiPath, const std::wstring& modFilesPath,
        std::wstring* outError = nullptr);
    bool PatchPluginsConfig(const std::wstring& kenshiPath);
    std::wstring FindKenshiPath();
    bool FetchLatestRelease(const char* owner, const char* repo, LatestRelease& out);
    std::wstring DownloadAndExtract(const std::string& url, const std::wstring& destDir);
    std::wstring GetLocalVPNIP();
    std::wstring GetLocalIP();
}

struct LatestRelease {
    std::string tagName;
    std::string zipUrl;
    std::string version;
};

enum class DeployResult {
    Success,
    KenshiNotFound,
    DownloadFailed,
    FileCopyFailed,
    ConfigPatchFailed
};