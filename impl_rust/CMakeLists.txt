cmake_minimum_required(VERSION 3.30)
project(ArgParseXRust)

add_rust_library(TARGET argpx_rs
        SOURCE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        BINARY_DIRECTORY "${CMAKE_BINARY_DIR}"
)
