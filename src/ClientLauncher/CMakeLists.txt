cmake_minimum_required(VERSION 3.20)
project(KenshiLauncherClient)

add_executable(ClientLauncher WIN32
    main.cpp
    ClientWindow.cpp
    resources.rc
)

target_link_libraries(ClientLauncher PRIVATE LauncherCommon ${WINDOWS_LIBS} ole32 comctl32 iphlpapi ws2_32 uuid)
target_compile_definitions(ClientLauncher PRIVATE _UNICODE UNICODE)
set_target_properties(ClientLauncher PROPERTIES OUTPUT_NAME "KenshiLauncher.Client")