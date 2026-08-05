cmake_minimum_required(VERSION 3.20)
project(linkscreen_harmony_native LANGUAGES CXX)

get_filename_component(
    LINKSCREEN_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.."
    ABSOLUTE
)

set(BUILD_TESTING OFF CACHE BOOL "Disable native protocol tests in HAP" FORCE)
add_subdirectory(
    "${LINKSCREEN_ROOT}/native/protocol"
    "${CMAKE_CURRENT_BINARY_DIR}/linkscreen_protocol"
)

add_library(entry SHARED napi_init.cpp)
target_compile_features(entry PRIVATE cxx_std_20)
target_compile_options(entry PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(entry PRIVATE linkscreen_protocol libace_napi.z.so)

