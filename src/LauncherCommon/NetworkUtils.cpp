#include "NetworkUtils.h"
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <cstdio>
#include <algorithm>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

// Suppress min/max macro conflicts
#ifndef NOMINMAX
#define NOMINMAX
#endif

namespace LauncherCommon {

static bool IsRadminVPNAdapter(const std::wstring& name, const std::wstring& ip) {
    std::wstring lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
    if (lowerName.find(L"radmin") != std::wstring::npos) return true;
    if (lowerName.find(L"r-vpn") != std::wstring::npos) return true;
    if (ip.rfind(L"100.", 0) == 0) return true;
    if (ip.rfind(L"10.", 0) == 0) return true;
    return false;
}

std::wstring GetLocalVPNIP() {
    auto ifs = GetNetworkInterfaces();
    for (const auto& iface : ifs) {
        if (iface.isVPN && !iface.ip.empty())
            return iface.ip;
    }
    return L"";
}

std::wstring GetLocalIP() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0)
        return L"127.0.0.1";

    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0)
        return L"127.0.0.1";

    char buf[64];
    for (addrinfo* p = result; p; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            auto* addr = reinterpret_cast<sockaddr_in*>(p->ai_addr);
            if (addr->sin_addr.S_un.S_un_b.s_b1 != 127) {
                inet_ntop(AF_INET, &addr->sin_addr, buf, sizeof(buf));
                freeaddrinfo(result);
                return std::wstring(buf, buf + strlen(buf));
            }
        }
    }
    freeaddrinfo(result);
    return L"127.0.0.1";
}

bool IsServerReachable(const char* ip, int port, int timeoutMs) {
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET) return false;

    DWORD timeout = (DWORD)timeoutMs;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int ret = connect(s, (sockaddr*)&addr, sizeof(addr));
    closesocket(s);
    return ret == 0;
}

std::vector<NetworkInterface> GetNetworkInterfaces() {
    std::vector<NetworkInterface> result;

    ULONG bufSize = 15000;
    std::vector<char> buf(bufSize);
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_FRIENDLY_NAME;

    DWORD dwRetVal = GetAdaptersAddresses(
        AF_INET, flags, nullptr,
        reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data()),
        &bufSize);

    if (dwRetVal != NO_ERROR) return result;

    PIP_ADAPTER_ADDRESSES pAddr = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data());
    while (pAddr) {
        NetworkInterface iface;
        iface.name = pAddr->FriendlyName ? std::wstring(pAddr->FriendlyName) : std::wstring();
        iface.ip = L"";
        iface.isVPN = false;

        PIP_ADAPTER_UNICAST_ADDRESS ua = pAddr->FirstUnicastAddress;
        while (ua) {
            if (ua->Address.lpSockaddr->sa_family == AF_INET) {
                auto* sin = reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
                char ipBuf[32];
                inet_ntop(AF_INET, &sin->sin_addr, ipBuf, sizeof(ipBuf));
                iface.ip = std::wstring(ipBuf, ipBuf + strlen(ipBuf));
                break;
            }
            ua = ua->Next;
        }

        if (!iface.ip.empty()) {
            iface.isVPN = IsRadminVPNAdapter(iface.name, iface.ip);
            result.push_back(iface);
        }

        pAddr = pAddr->Next;
    }

    return result;
}

} // namespace LauncherCommon