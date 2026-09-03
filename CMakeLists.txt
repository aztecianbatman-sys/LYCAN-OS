cmake_minimum_required(VERSION 3.24)
project(LYCAN VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(LYCAN_BUILD_TESTS "Build LYCAN tests" ON)
option(LYCAN_BUILD_CLI "Build the LYCAN CLI" ON)
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
 src/recovery/recovery_manager.cpp)
target_include_directories(lycan_core PUBLIC src)
target_compile_definitions(lycan_core PUBLIC LYCAN_VERSION="1.0.0")
if(WIN32)
 add_executable(lycan-vm WIN32 src/main.cpp src/platform/win32_desktop.cpp)
 target_link_libraries(lycan-vm PRIVATE lycan_core user32 gdi32 shell32 advapi32 winhttp)
 target_compile_definitions(lycan-vm PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
 set_target_properties(lycan-vm PROPERTIES OUTPUT_NAME "lycan-vm")
else()
 add_executable(lycan-vm src/main.cpp)
 target_link_libraries(lycan-vm PRIVATE lycan_core)
endif()
if(LYCAN_BUILD_CLI)
 add_executable(lycan-cli tools/lycan_cli.cpp)
 target_link_libraries(lycan-cli PRIVATE lycan_core)
endif()
if(LYCAN_BUILD_TESTS)
 enable_testing()
 add_executable(lycan-tests tests/test_vm.cpp)
 target_link_libraries(lycan-tests PRIVATE lycan_core)
 add_test(NAME lycan-tests COMMAND lycan-tests)
 add_executable(lycan-feature-tests tests/test_features.cpp)
 target_link_libraries(lycan-feature-tests PRIVATE lycan_core)
 add_test(NAME lycan-feature-tests COMMAND lycan-feature-tests)
 add_executable(lycan-runtime-tests tests/test_runtime.cpp)
 target_link_libraries(lycan-runtime-tests PRIVATE lycan_core)
 add_test(NAME lycan-runtime-tests COMMAND lycan-runtime-tests)
endif()
install(TARGETS lycan-vm RUNTIME DESTINATION bin)
if(LYCAN_BUILD_CLI)
 install(TARGETS lycan-cli RUNTIME DESTINATION bin)
endif()
install(FILES tools/lycan_sdk.py DESTINATION sdk)
install(DIRECTORY store/ DESTINATION store)
install(DIRECTORY docs/ DESTINATION docs)
install(DIRECTORY installer/ DESTINATION installer)
