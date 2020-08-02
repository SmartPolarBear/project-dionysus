cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_subdirectory(console)
add_subdirectory(maths)

target_sources(kernel
    PRIVATE memory.cc)

target_include_directories(kernel PUBLIC include)