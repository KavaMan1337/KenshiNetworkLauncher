#include "ServerProcess.h"
#include <windows.h>
#include <string>
#include <thread>
#include <vector>

namespace HostLauncher {

struct ServerProcess::Impl {
    HANDLE hProcess = nullptr;
    HANDLE hReadPipe = nullptr;
    std::thread readerThread;
    bool running = false;
};

ServerProcess::ServerProcess() : p(new Impl()) {}
ServerProcess::~ServerProcess() { Stop(); delete p; }

bool ServerProcess::Start(const std::wstring& exePath,
                          const std::wstring& workingDir,
                          std::function<void(const char*, int)> onOutput) {
    if (p->running) return false;

    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hWritePipe;
    if (!CreatePipe(&p->hReadPipe, &hWritePipe, &sa, 0)) return false;
    SetHandleInformation(p->hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(exePath.c_str(), nullptr, nullptr, nullptr,
            TRUE, CREATE_NO_WINDOW, nullptr, workingDir.c_str(), &si, &pi)) {
        CloseHandle(p->hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }

    CloseHandle(hWritePipe);
    CloseHandle(pi.hThread);
    p->hProcess = pi.hProcess;
    p->running = true;

    p->readerThread = std::thread([this, onOutput]() {
        char buf[4096];
        DWORD bytesRead;
        while (p->running) {
            if (!ReadFile(p->hReadPipe, buf, sizeof(buf) - 1, &bytesRead, nullptr)
                    || bytesRead == 0) {
                break;
            }
            buf[bytesRead] = '\0';
            if (onOutput) onOutput(buf, (int)bytesRead);
        }
    });

    return true;
}

void ServerProcess::Stop() {
    if (!p->running) return;
    p->running = false;

    if (p->hProcess) {
        TerminateProcess(p->hProcess, 0);
        WaitForSingleObject(p->hProcess, 5000);
        CloseHandle(p->hProcess);
        p->hProcess = nullptr;
    }

    if (p->hReadPipe) {
        CloseHandle(p->hReadPipe);
        p->hReadPipe = nullptr;
    }

    if (p->readerThread.joinable())
        p->readerThread.join();
}

bool ServerProcess::IsRunning() const {
    if (!p->hProcess) return false;
    DWORD code;
    return GetExitCodeProcess(p->hProcess, &code) && code == STILL_ACTIVE;
}

}
