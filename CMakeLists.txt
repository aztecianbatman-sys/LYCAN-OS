cmake_minimum_required(VERSION 3.24)
project(LYCAN VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(LYCAN_BUILD_TESTS "Build tests" ON)

add_library(lycan_core
    src/core/ares_vm.cpp
    src/core/ares_hardware.cpp
    src/core/lyfs.cpp
    src/core/process.cpp
    src/core/security.cpp
    src/core/snapshot.cpp
    src/core/settings.cpp
    src/package/package_archive.cpp
    src/package/package_manager_fixed.cpp
    src/runtime/app_manager.cpp
    src/runtime/app_host.cpp
    src/runtime/app_runtime.cpp
    src/runtime/gecko_runtime.cpp
    src/runtime/window_manager.cpp
    src/store/store_client.cpp
    src/recovery/recovery_manager.cpp
)
target_include_directories(lycan_core PUBLIC src)
target_compile_definitions(lycan_core PUBLIC LYCAN_VERSION="1.0.0")
if(WIN32)
    target_link_libraries(lycan_core PUBLIC winhttp)
endif()

add_executable(lycan-backend src/backend/main.cpp)
target_link_libraries(lycan-backend PRIVATE lycan_core)

if(WIN32)
    target_compile_definitions(lycan-backend PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(LYCAN_BUILD_TESTS)
    enable_testing()
    add_executable(lycan-runtime-tests tests/test_runtime.cpp)
    target_link_libraries(lycan-runtime-tests PRIVATE lycan_core)
    add_test(NAME lycan-runtime-tests COMMAND lycan-runtime-tests)
endif()

install(TARGETS lycan-backend RUNTIME DESTINATION bin)
install(DIRECTORY frontend/ DESTINATION frontend)
