#pragma once
#include <windows.h>
#include <string>
#include <functional>

namespace HostLauncher {

class ServerProcess {
    struct Impl;
    Impl* p;
public:
    ServerProcess();
    ~ServerProcess();

    // Запускает процесс. onOutput вызывается при каждой строке вывода.
    // Возвращает true при успехе.
    bool Start(const std::wstring& exePath,
               const std::wstring& workingDir,
               std::function<void(const char*, int)> onOutput);

    // Останавливает процесс
    void Stop();

    // Возвращает true если процесс ещё работает
    bool IsRunning() const;
};

} // namespace