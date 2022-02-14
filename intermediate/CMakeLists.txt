cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

set(bsoncxx_ROOT /opt/homebrew/Cellar/mongo-cxx-driver)
set(mongocxx_ROOT /opt/homebrew/Cellar/mongo-cxx-driver)

find_package(bsoncxx REQUIRED CONFIG)
find_package(mongocxx REQUIRED CONFIG)

add_library(database STATIC database/database.cpp)
set_target_properties(database PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/intermediate/database/include")

target_link_libraries(database mongo::bsoncxx_shared mongo::mongocxx_shared)

add_executable(register registration/register.cpp registration/main.cpp)
set_target_properties(register PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/intermediate/registration/include")

target_link_libraries(register database nlohmann_json::nlohmann_json utils)

add_executable(balancer balancer/balancer.cpp balancer/main.cpp)
set_target_properties(balancer PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/intermediate/balancer/include")

target_link_libraries(balancer nlohmann_json::nlohmann_json utils)