cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(mkramdisk)

set(CMAKE_CXX_STANDARD 20)

add_executable(mkramdisk main.cc)

target_link_libraries(mkramdisk ramdisk)

target_link_libraries(mkramdisk GSL)