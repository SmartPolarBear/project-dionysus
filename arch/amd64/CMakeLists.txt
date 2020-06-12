cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_library(arch_amd64 STATIC cpu.S)

target_include_directories(arch_amd64 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

set_property(SOURCE cpu.S PROPERTY LANGUAGE C)

target_sources(arch_amd64 PRIVATE cpu.S)
