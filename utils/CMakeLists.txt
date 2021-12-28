cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 14)

add_library(network INTERFACE)
target_include_directories(network INTERFACE "${PROJECT_SOURCE_DIR}/utils/include")
