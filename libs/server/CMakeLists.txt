cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_library(server STATIC entry.S entry_main.cc)

add_subdirectory(syscall)
add_subdirectory(memory)

set_property(SOURCE entry.S PROPERTY LANGUAGE C)

target_include_directories(server PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_include_directories(server PUBLIC "include")
target_include_directories(server PRIVATE "include_private")

target_include_directories(server
        INTERFACE "${CMAKE_BINARY_DIR}/external/third_party_root/include/c++/v1"
        INTERFACE "${CMAKE_BINARY_DIR}/external/third_party_root/include/sys"
        INTERFACE "${CMAKE_BINARY_DIR}/external/third_party_root/include")

target_sources(server PRIVATE entry.S entry_main.cc)

target_link_directories(server BEFORE PRIVATE ${CMAKE_BINARY_DIR}/external/third_party_root/lib)

add_dependencies(server libcxx_build libcxxabi_build newlib_build)

target_link_directories(server INTERFACE ${CMAKE_BINARY_DIR}/external/third_party_root/lib)

target_link_libraries(server c g m c++ c++abi c++experimental)

target_compile_options(server BEFORE
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