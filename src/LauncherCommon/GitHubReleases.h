#pragma once
#include <string>
#include <vector>

namespace LauncherCommon {

struct ReleaseAsset {
    std::string name;      // filename
    std::string downloadUrl; // browser_download_url
};

struct LatestRelease {
    std::string tagName;   // e.g. "v1.2.3"
    std::string zipUrl;    // URL to download the zip
    std::string version;  // human-readable
};

// Скачивает информацию о последнем релизе Kenshi-Online с GitHub API
// Возвращает false при ошибке
bool FetchLatestRelease(const char* owner, const char* repo, LatestRelease& out);

// Скачивает ZIP архив релиза во временную папку и распаковывает в destDir
// Возвращает путь к папке с распакованными файлами или пустую строку
std::wstring DownloadAndExtract(const std::string& url, const std::wstring& destDir);

// Скачивает произвольный URL в файл
bool DownloadFile(const std::string& url, const std::wstring& destPath);

}