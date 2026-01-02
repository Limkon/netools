cmake_minimum_required(VERSION 3.16)

project(NetToolPro VERSION 3.7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 修改点 1: 查找 Qt5 而不是 Qt6 ---
find_package(Qt5 REQUIRED COMPONENTS Widgets Network Core)

set(PROJECT_RESOURCES
    resources/resources.qrc
)

set(WIN_RC_FILE "")
if(WIN32)
    set(WIN_RC_FILE resources/NetToolPro.rc)
endif()

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

if(WIN32)
    list(APPEND SYSTEM_LIBS Iphlpapi Ws2_32 Advapi32 Wininet)
endif()

add_executable(NetToolPro WIN32
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

# --- 修改点 2: 链接 Qt5 库 ---
target_link_libraries(NetToolPro PRIVATE
    Qt5::Widgets
    Qt5::Network
    Qt5::Core
    ${SYSTEM_LIBS}
)

# Qt5 需要手动开启这些
set_property(TARGET NetToolPro PROPERTY CXX_STANDARD 17)
set_property(TARGET NetToolPro PROPERTY AUTOMOC ON)
set_property(TARGET NetToolPro PROPERTY AUTOUIC ON)
set_property(TARGET NetToolPro PROPERTY AUTORCC ON)
