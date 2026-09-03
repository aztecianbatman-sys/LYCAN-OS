cmake_minimum_required(VERSION 3.25)
project(LycanOS VERSION 0.5.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(LYCAN_BUILD_TESTS "Build Lycan automated tests" ON)

add_library(lycan_core STATIC
    src/lycan_platform.cpp
    src/lycan_store.cpp
)
target_include_directories(lycan_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(lycan_core PUBLIC LYCAN_VERSION=\"0.5.0\")
if(WIN32)
    target_link_libraries(lycan_core PRIVATE winhttp)
endif()

if(WIN32)
    add_executable(lycan-vm WIN32 main.cpp)
    target_link_libraries(lycan-vm PRIVATE lycan_core user32 gdi32)
else()
    add_executable(lycan-vm main.cpp)
    target_link_libraries(lycan-vm PRIVATE lycan_core)
endif()

add_executable(lycan-cli tools/lycan_cli.cpp)
target_link_libraries(lycan-cli PRIVATE lycan_core)

if(LYCAN_BUILD_TESTS)
    enable_testing()
    add_executable(lycan-tests tests/test_main.cpp)
    target_link_libraries(lycan-tests PRIVATE lycan_core)
    add_test(NAME lycan-tests COMMAND lycan-tests)
endif()
