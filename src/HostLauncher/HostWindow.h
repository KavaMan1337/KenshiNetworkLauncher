#pragma once
#include <windows.h>
#include <string>
#include <functional>

// Колбэки для обновления UI из рабочих потоков
struct HostWindowCallbacks {
    std::function<void(const wchar_t*)> onLog;           // добавить строку в лог
    std::function<void(bool running)> onServerState;      // изменилось состояние сервера
    std::function<void(const wchar_t* ip, int port)> onServerInfo; // сервер запущен, показать IP
    std::function<void()> onInstallComplete;              // мод установлен
    std::function<void(const wchar_t* err)> onError;      // ошибка
    std::function<void(int progress)> onProgress;         // прогресс скачивания 0-100
};

// Создаёт и показывает главное окно хоста
// Возвращает дескриптор окна
HWND CreateHostWindow(HINSTANCE hInstance, HostWindowCallbacks callbacks);

// Запускает цикл обработки сообщений (блокирующий)
int RunHostWindow(HINSTANCE hInstance);

// Функции для изменения состояния UI извне
void HostWindowSetStatus(HWND hwnd, const wchar_t* status);
void HostWindowSetProgress(HWND hwnd, int progress);
void HostWindowAddLog(HWND hwnd, const wchar_t* line);
void HostWindowSetServerRunning(HWND hwnd, bool running);
void HostWindowSetServerIP(HWND hwnd, const wchar_t* ip, int port);
void HostWindowNotifyInstallComplete(HWND hwnd);
void HostWindowNotifyError(HWND hwnd, const wchar_t* err);
