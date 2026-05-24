#include <windows.h>
#include <commctrl.h>
#include <objbase.h>

#include "HostWindow.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    InitCommonControls();

    int ret = RunHostWindow(hInstance);

    CoUninitialize();
    return ret;
}
