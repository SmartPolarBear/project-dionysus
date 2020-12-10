cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_library(ktl INTERFACE)
target_include_directories(ktl INTERFACE include/)

add_dependencies(ktl c g m c++abi c++ c++experimental)
