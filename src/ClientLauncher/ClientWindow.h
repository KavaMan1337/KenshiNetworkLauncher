#pragma once
#include <windows.h>
#include <string>
#include <functional>

// Колбэки для обновления UI из рабочих потоков
struct ClientWindowCallbacks {
    std::function<void(const wchar_t*)> onStatus;
    std::function<void(bool)> onCanPlay;  // активировать кнопку "Играть"
    std::function<void(const wchar_t*)> onError;
    std::function<void(int)> onProgress;
};

// Создаёт и показывает главное окно клиента
int RunClientWindow(HINSTANCE hInstance);

// Уведомления из других потоков
void ClientWindowSetModInstalled(HWND hwnd);
void ClientWindowSetProgress(HWND hwnd, int p);
void ClientWindowSetStatus(HWND hwnd, const wchar_t* status);
void ClientWindowEnablePlay(HWND hwnd, bool enable);
void ClientWindowNotifyError(HWND hwnd, const wchar_t* err);