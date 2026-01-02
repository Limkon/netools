cmake_minimum_required(VERSION 3.16)

project(NetToolPro VERSION 3.7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Core)

# --- 资源文件配置 ---
set(PROJECT_RESOURCES
    resources/resources.qrc
)

set(WIN_RC_FILE "")
if(WIN32)
    set(WIN_RC_FILE resources/NetToolPro.rc)
endif()
# -------------------

set(PROJECT_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/WorkerThreads.cpp
    src/ProxyManager.cpp
    ${PROJECT_RESOURCES}
    ${WIN_RC_FILE}
)

set(PROJECT_HEADERS
    src/MainWindow.h
    src/WorkerThreads.h
    src/ProxyManager.h
)

# --- 修复点：添加 Wininet 库 ---
if(WIN32)
    # Iphlpapi: 用于 ICMP Ping
    # Ws2_32:   用于 Winsock 网络连接
    # Advapi32: 用于 注册表操作
    # Wininet:  用于 刷新系统代理设置 (解决 LNK2019 错误)
    list(APPEND SYSTEM_LIBS Iphlpapi Ws2_32 Advapi32 Wininet)
endif()
# -----------------------------

add_executable(NetToolPro WIN32
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

target_link_libraries(NetToolPro PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Core
    ${SYSTEM_LIBS}
)

set_property(TARGET NetToolPro PROPERTY CXX_STANDARD 17)
set_property(TARGET NetToolPro PROPERTY AUTOMOC ON)
set_property(TARGET NetToolPro PROPERTY AUTOUIC ON)
set_property(TARGET NetToolPro PROPERTY AUTORCC ON)
