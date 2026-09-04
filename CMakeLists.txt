cmake_minimum_required(VERSION 3.24)
project(LYCAN VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(lycan_vm
  src/vm/vm.cpp
  src/vm/fs.cpp
  src/network/network.cpp
)
target_include_directories(lycan_vm PUBLIC src)

add_executable(lycan-backend src/backend/main.cpp)
target_link_libraries(lycan-backend PRIVATE lycan_vm)
if(WIN32)
  target_compile_definitions(lycan-backend PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

option(LYCAN_BUILD_TESTS "Build tests" ON)
if(LYCAN_BUILD_TESTS)
  enable_testing()
  add_executable(lycan-tests tests/test_vm.cpp)
  target_link_libraries(lycan-tests PRIVATE lycan_vm)
  add_test(NAME lycan-tests COMMAND lycan-tests)

  add_executable(lycan-network-tests tests/test_network.cpp)
  target_link_libraries(lycan-network-tests PRIVATE lycan_vm)
  add_test(NAME lycan-network-tests COMMAND lycan-network-tests)
endif()
