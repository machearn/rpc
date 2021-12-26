cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 14)

include(FetchContent)
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
  GIT_TAG v3.10.4)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_library(rpc_client INTERFACE)

target_include_directories(rpc_client INTERFACE "${PROJECT_SOURCE_DIR}/client/include")

target_link_libraries(rpc_client INTERFACE nlohmann_json::nlohmann_json)

# add rpc_clangd target to generate compile command for clangd
# indexing nlhomann/json
add_executable(rpc_clangd "${PROJECT_SOURCE_DIR}/client/main.cpp")

target_link_libraries(rpc_clangd rpc_client)
