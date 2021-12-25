cmake_minimum_required(VERSION 3.14)

project(rpc)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory("client")

add_executable(run_test main.cpp)
target_link_libraries(run_test rpc_client)
