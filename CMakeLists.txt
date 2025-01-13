cmake_minimum_required(VERSION 3.30)
project(ArgParseX)

set(CMAKE_EXECUTABLE_SUFFIX ".out")

set(RUSTC_MINIMUM_REQUIRED 1.56)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include_directories("${PROJECT_SOURCE_DIR}/include")

find_package(Rust REQUIRED)

add_subdirectory("${PROJECT_SOURCE_DIR}/impl_rust")
add_subdirectory("${PROJECT_SOURCE_DIR}/impl_c")

add_subdirectory("${PROJECT_SOURCE_DIR}/example")
