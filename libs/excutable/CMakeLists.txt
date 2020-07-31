cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_library(executable STATIC elf.cc)

target_include_directories(executable PUBLIC "include")
target_include_directories(executable PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_include_directories(executable BEFORE
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root/include/sys"
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root/include"
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root/include/c++/v1")

add_dependencies(executable libcxx_build libcxxabi_build newlib_build)

target_link_directories(executable INTERFACE ${CMAKE_BINARY_DIR}/external/third_party_root/lib)

target_link_libraries(executable c g m c++ c++abi c++experimental)

target_compile_options(executable BEFORE
        PRIVATE -g
        PRIVATE --target=x86_64-pc-linux-elf
        PRIVATE -fno-pie
        PRIVATE -fno-exceptions
        PRIVATE -fno-rtti
        PRIVATE -fno-stack-protector
        PRIVATE -nostdlib
        PRIVATE -ffreestanding
        PRIVATE -fno-builtin
        PRIVATE -Wall
        PRIVATE -Wno-gnu-include-next
        PRIVATE -Wextra
        PRIVATE -march=x86-64
        PRIVATE -mtls-direct-seg-refs
        PRIVATE -mno-sse
        PRIVATE -msoft-float
        PRIVATE -mcmodel=large
        PRIVATE -mno-red-zone
        PRIVATE -nostdlibinc
        PRIVATE -D__ELF__
        PRIVATE -D_LDBL_EQ_DBL
        PRIVATE -D_GNU_SOURCE
        PRIVATE -D_POSIX_TIMERS)