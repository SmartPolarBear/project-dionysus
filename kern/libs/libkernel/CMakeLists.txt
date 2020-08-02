cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_subdirectory(io)
add_subdirectory(maths)

target_include_directories(kernel PUBLIC include)