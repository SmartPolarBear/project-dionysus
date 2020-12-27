cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_include_directories(kernel PRIVATE include)

add_subdirectory(lock)