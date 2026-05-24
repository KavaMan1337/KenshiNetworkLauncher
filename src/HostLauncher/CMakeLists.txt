cmake_minimum_required(VERSION 3.20)
project(KenshiLauncherHost)

add_executable(HostLauncher WIN32
    main.cpp
    HostWindow.cpp
    ServerProcess.cpp
    resources.rc
)

target_link_libraries(HostLauncher PRIVATE LauncherCommon ${WINDOWS_LIBS} ole32 comctl32 iphlpapi ws2_32 uuid)
target_compile_definitions(HostLauncher PRIVATE _UNICODE UNICODE)
set_target_properties(HostLauncher PROPERTIES OUTPUT_NAME "KenshiLauncher.Host")
