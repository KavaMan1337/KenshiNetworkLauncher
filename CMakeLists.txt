cmake_minimum_required(VERSION 3.20)
project(KenshiNetworkLauncher CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8 /EHsc>)
add_compile_options($<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>)

find_package(OpenSSL QUIET)

# Windows SDK libs
if(MSVC)
    set(WINDOWS_LIBS winhttp shlwapi comctl32 ole32 oleaut32 uuid iphlpapi ws2_32)
else()
    set(WINDOWS_LIBS winhttp shlwapi comctl32 ole32 oleaut32 uuid iphlpapi ws2_32)
endif()

# LauncherCommon: static lib with shared code
add_subdirectory(src/LauncherCommon)

# HostLauncher: GUI server launcher
add_subdirectory(src/HostLauncher)

# ClientLauncher: GUI client launcher
add_subdirectory(src/ClientLauncher)
