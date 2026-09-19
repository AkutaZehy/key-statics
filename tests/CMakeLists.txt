find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test Widgets Network)

set(CMAKE_SOURCE_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_executable(test_keystats
    test_keystats.cpp
    ${CMAKE_SOURCE_SRC_DIR}/keystats.cpp
    ${CMAKE_SOURCE_SRC_DIR}/keystats.h
)
target_include_directories(test_keystats PRIVATE ${CMAKE_SOURCE_SRC_DIR})
target_link_libraries(test_keystats PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Core)

add_executable(test_keylayout
    test_keylayout.cpp
    ${CMAKE_SOURCE_SRC_DIR}/keylayout.cpp
    ${CMAKE_SOURCE_SRC_DIR}/keylayout.h
)
target_include_directories(test_keylayout PRIVATE ${CMAKE_SOURCE_SRC_DIR})
target_link_libraries(test_keylayout PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Core)

add_executable(test_config
    test_config.cpp
    ${CMAKE_SOURCE_SRC_DIR}/config.cpp
    ${CMAKE_SOURCE_SRC_DIR}/config.h
)
target_include_directories(test_config PRIVATE ${CMAKE_SOURCE_SRC_DIR})
target_link_libraries(test_config PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)

add_executable(test_gamepad
    test_gamepad.cpp
    ${CMAKE_SOURCE_SRC_DIR}/gamepadpoller.cpp
    ${CMAKE_SOURCE_SRC_DIR}/gamepadpoller.h
)
target_include_directories(test_gamepad PRIVATE ${CMAKE_SOURCE_SRC_DIR})
target_link_libraries(test_gamepad PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Core)

add_executable(test_virtualkeyboard
    test_virtualkeyboard.cpp
    ${CMAKE_SOURCE_SRC_DIR}/virtualkeyboard.cpp
    ${CMAKE_SOURCE_SRC_DIR}/virtualkeyboard.h
    ${CMAKE_SOURCE_SRC_DIR}/keylayout.cpp
    ${CMAKE_SOURCE_SRC_DIR}/keylayout.h
    ${CMAKE_SOURCE_SRC_DIR}/config.cpp
    ${CMAKE_SOURCE_SRC_DIR}/config.h
)
target_include_directories(test_virtualkeyboard PRIVATE ${CMAKE_SOURCE_SRC_DIR})
target_compile_definitions(test_virtualkeyboard PRIVATE KEY_STATICS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(test_virtualkeyboard PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)

add_test(NAME test_keystats COMMAND test_keystats)
add_test(NAME test_keylayout COMMAND test_keylayout)
add_test(NAME test_config COMMAND test_config)
add_test(NAME test_gamepad COMMAND test_gamepad)
add_test(NAME test_virtualkeyboard COMMAND test_virtualkeyboard)
