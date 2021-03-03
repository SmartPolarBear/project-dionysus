cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_subdirectory(job)
add_subdirectory(process)
add_subdirectory(thread)
add_subdirectory(scheduler)
add_subdirectory(ipc)

target_include_directories(kernel PRIVATE include)