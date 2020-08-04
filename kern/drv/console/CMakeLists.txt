cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_subdirectory(cga)
add_subdirectory(console)
add_subdirectory(uart)


target_include_directories(kernel PRIVATE include)
