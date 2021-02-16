cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_include_directories(kernel PRIVATE include)

# TODO: more elegant way to do this
target_include_directories(arch_amd64 PRIVATE include)