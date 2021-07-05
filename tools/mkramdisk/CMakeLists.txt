cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(mkramdisk)

set(CMAKE_CXX_STANDARD 20)

add_executable(mkramdisk main.cc config.cc)

target_include_directories(mkramdisk PRIVATE include)

target_compile_options(mkramdisk PUBLIC -stdlib=libc++)

target_link_options(mkramdisk PUBLIC -stdlib=libc++)

target_link_libraries(mkramdisk
        PRIVATE ramdisk
        PRIVATE GSL)
