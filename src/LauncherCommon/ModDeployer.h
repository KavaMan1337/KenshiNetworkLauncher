#pragma once
#include <string>
#include <vector>

namespace LauncherCommon {

enum class DeployResult {
    Success,
    KenshiNotFound,
    DownloadFailed,
    FileCopyFailed,
    ConfigPatchFailed
};

// Проверяет установлен ли мод (KenshiMP.Core.dll в папке игры)
bool IsModInstalled(const std::wstring& kenshiPath);

// Копирует файлы мода в папку Kenshi
// kenshiPath — путь к папке игры
// modFilesPath — путь к папке с файлами мода (из распакованного архива)
// Возвращает результат операции
DeployResult DeployModFiles(const std::wstring& kenshiPath, const std::wstring& modFilesPath,
    std::wstring* outError = nullptr);

// Читает Plugins_x64.cfg и добавляет Plugin=KenshiMP.Core если отсутствует
bool PatchPluginsConfig(const std::wstring& kenshiPath);

}