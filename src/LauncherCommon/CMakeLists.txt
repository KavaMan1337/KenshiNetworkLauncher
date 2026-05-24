cmake_minimum_required(VERSION 3.20)
project(KenshiLauncherCommon)

add_library(LauncherCommon STATIC
    SteamPath.cpp
    GitHubReleases.cpp
    ModDeployer.cpp
    NetworkUtils.cpp
)

target_include_directories(LauncherCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(LauncherCommon PUBLIC ${WINDOWS_LIBS} winhttp shlwapi ole32 comctl32 iphlpapi ws2_32 uuid)
target_compile_definitions(LauncherCommon PRIVATE _UNICODE UNICODE)
