#include <windows.h>
#include "HostWindow.h"

INT_PTR WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                        LPWSTR lpCmdLine, INT nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // Регистрируем класс главного окна
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefDlgProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"KenshiHostWindow";
    RegisterClassExW(&wc);

    InitCommonControls();

    int ret = RunHostWindow(hInstance);

    CoUninitialize();
    return ret;
}