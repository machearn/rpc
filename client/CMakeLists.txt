cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 14)

add_library(rpc_client INTERFACE)

target_include_directories(rpc_client INTERFACE "${PROJECT_SOURCE_DIR}/client/include")

target_link_libraries(rpc_client INTERFACE nlohmann_json::nlohmann_json network)

# add rpc_clangd target to generate compile command for clangd
# indexing nlhomann/json
add_executable(rpc_clangd "${PROJECT_SOURCE_DIR}/client/main.cpp")

target_link_libraries(rpc_clangd rpc_client)
