cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_subdirectory(ext2)
add_subdirectory(vfs)

target_sources(kernel
        PRIVATE fs.cc)
