cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_subdirectory(inode)
add_subdirectory(block)
add_subdirectory(directory)

target_sources(kernel
        PRIVATE ext2.cc
        PRIVATE vnode.cc)
