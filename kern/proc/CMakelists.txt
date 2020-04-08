cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_subdirectory(syscall)
add_subdirectory(process_manager)


target_include_directories(kernel PRIVATE include)