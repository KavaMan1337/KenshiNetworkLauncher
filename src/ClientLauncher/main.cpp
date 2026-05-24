#include <windows.h>
#include "ClientWindow.h"

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, INT nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    InitCommonControls();
    int ret = RunClientWindow(hInstance);
    CoUninitialize();
    return ret;
}