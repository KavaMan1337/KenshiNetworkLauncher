#pragma once
#include <string>

namespace LauncherCommon {

// Получает локальный IP адрес для Radmin VPN адаптера
// Возвращает строку типа "100.100.0.1" или пустую строку
std::wstring GetLocalVPNIP();

// Получает IP для произвольного подключения
std::wstring GetLocalIP();

// Проверяет доступность сервера
bool IsServerReachable(const char* ip, int port, int timeoutMs = 1000);

// Возвращает список доступных сетевых интерфейсов
struct NetworkInterface {
    std::wstring name;
    std::wstring ip;
    bool isVPN;
};
std::vector<NetworkInterface> GetNetworkInterfaces();

}