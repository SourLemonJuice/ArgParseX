cmake_minimum_required(VERSION 3.30)
project(ArgParseX_C C)

set(CMAKE_C_STANDARD 11)

add_library(argpx_c STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/argpx.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/argpx_hash.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/argpx_hash.h"
)

#target_include_directories(argpx_c PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_link_libraries(argpx_c PRIVATE argpx_rs)
