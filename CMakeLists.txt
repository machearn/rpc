cmake_minimum_required(VERSION 3.14)

project(rpc)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(json
        GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
        GIT_TAG v3.10.4)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_subdirectory("client")
add_subdirectory("test")
add_subdirectory("utils")
add_subdirectory("intermediate")

enable_testing()

add_executable(serialization_test test/serialization_test.cpp)
target_link_libraries(serialization_test rpc_client gtest_main)

add_executable(database_test test/database_test.cpp)
target_link_libraries(database_test database gtest_main)

include(GoogleTest)
gtest_discover_tests(serialization_test)
gtest_discover_tests(database_test)
