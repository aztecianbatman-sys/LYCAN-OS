cmake_minimum_required(VERSION 3.25)
project(LycanOS VERSION 0.4.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(LYCAN_BUILD_TESTS "Build Lycan automated tests" ON)
option(LYCAN_HEADLESS "Disable native host window and run headless" OFF)
option(LYCAN_ENABLE_GECKO "Enable externally supplied Gecko integration" OFF)
add_library(lycan_core STATIC
 vm/cpu/ares_cpu.cpp vm/cpu/assembler.cpp vm/memory/virtual_memory.cpp
 vm/disk/virtual_disk.cpp vm/disk/lyfs.cpp vm/display/framebuffer.cpp
 vm/debugger/debugger.cpp vm/network/network.cpp vm/machine_manager.cpp vm/vm.cpp
 firmware/boot_rom.cpp bootloader/bootloader.cpp kernel/kernel.cpp
 kernel/process/process_manager.cpp kernel/scheduler/scheduler.cpp security/security.cpp
 sdk/lycan_api.cpp kernel/memory/memory_manager.cpp kernel/filesystem/filesystem.cpp
 kernel/syscall/syscall.cpp kernel/drivers/device_manager.cpp packages/package_manager.cpp
 apps/crawford/crawford_host.cpp browser/gecko/gecko_adapter.cpp)
target_include_directories(lycan_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(lycan_core PUBLIC LYCAN_VERSION=\"0.4.1\")
if(WIN32)
 target_compile_definitions(lycan_core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
if(LYCAN_ENABLE_GECKO)
 target_compile_definitions(lycan_core PUBLIC LYCAN_ENABLE_GECKO=1)
endif()
if(WIN32 AND NOT LYCAN_HEADLESS)
 add_executable(lycan-vm WIN32 main.cpp gui/win32_host.cpp)
 target_link_libraries(lycan-vm PRIVATE lycan_core user32 gdi32 dwmapi shcore)
else()
 add_executable(lycan-vm main.cpp gui/headless_host.cpp)
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
install(TARGETS lycan-vm lycan-cli DESTINATION bin)
