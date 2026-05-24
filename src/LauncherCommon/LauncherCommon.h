#pragma once
#include <string>
#include <vector>
#include <functional>

namespace LauncherCommon {

enum class DeployResult {
    Success,
    KenshiNotFound,
    DownloadFailed,
    FileCopyFailed,
    ConfigPatchFailed
};

struct LatestRelease {
    std::string tagName;
    std::string zipUrl;
    std::string version;
};

// Steam
std::wstring FindKenshiPath();

// GitHub
bool FetchLatestRelease(const char* owner, const char* repo, LatestRelease& out);
std::wstring DownloadAndExtract(const std::string& url, const std::wstring& destDir);
bool DownloadFile(const std::string& url, const std::wstring& destPath);

// Mod deployment
bool IsModInstalled(const std::wstring& kenshiPath);
DeployResult DeployModFiles(const std::wstring& kenshiPath, const std::wstring& modFilesPath,
    std::wstring* outError = nullptr);
bool PatchPluginsConfig(const std::wstring& kenshiPath);

// Network
std::wstring GetLocalVPNIP();
std::wstring GetLocalIP();
bool IsServerReachable(const char* ip, int port, int timeoutMs = 1000);

struct NetworkInterface {
    std::wstring name;
    std::wstring ip;
    bool isVPN;
};
std::vector<NetworkInterface> GetNetworkInterfaces();

} // namespace