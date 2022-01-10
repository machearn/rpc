cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 14)

add_library(utils INTERFACE)
target_include_directories(utils INTERFACE "${PROJECT_SOURCE_DIR}/utils/include")