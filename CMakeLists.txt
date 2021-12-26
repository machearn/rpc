cmake_minimum_required(VERSION 3.14)

project(rpc)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory("client")
add_subdirectory("test")

enable_testing()

add_executable(serialization_test test/serialization_test.cpp)

target_link_libraries(serialization_test rpc_client gtest_main)

include(GoogleTest)
gtest_discover_tests(serialization_test)
