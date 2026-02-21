cmake_minimum_required(VERSION 3.16)
project(key-statics VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_BUILD_TYPE Release)

set(QT_VERSION_MAJOR 6)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

set(HEADERS
    src/keyboardhook.h
    src/mousehook.h
    src/keylayout.h
    src/virtualkeyboard.h
    src/mainwindow.h
    src/systray.h
    src/keystats.h
    src/httpserver.h
    src/config.h
    src/previewwindow.h
)

set(SOURCES
    src/keyboardhook.cpp
    src/mousehook.cpp
    src/keylayout.cpp
    src/virtualkeyboard.cpp
    src/mainwindow.cpp
    src/systray.cpp
    src/keystats.cpp
    src/httpserver.cpp
    src/config.cpp
    src/previewwindow.cpp
    src/main.cpp
    resources.rc
)

add_executable(${PROJECT_NAME} WIN32
    ${HEADERS}
    ${SOURCES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    ICON "assets/key-statics.ico"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        winmm.lib
        user32.lib
    )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
)
