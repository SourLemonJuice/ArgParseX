cmake_minimum_required(VERSION 3.30)
project(example1)

add_executable(TestC ./test.c)
add_executable(AFFI ./a.c)

#target_include_directories(TestC PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
#target_include_directories(AFFI PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")

target_link_libraries(TestC PUBLIC argpx_c argpx_rs)
target_link_libraries(AFFI PUBLIC argpx_c argpx_rs)
