cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

add_library(ramdisk INTERFACE)

target_include_directories(ramdisk INTERFACE include)
