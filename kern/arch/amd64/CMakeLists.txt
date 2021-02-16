cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_library(arch_amd64 STATIC cpu/cpu.S)

if (KERNEL_ENABLE_DEBUG_FACILITY)
    target_compile_options(arch_amd64 BEFORE
            PRIVATE -D_KERNEL_ENABLE_DEBUG_FACILITY)
endif ()

target_include_directories(arch_amd64 PRIVATE "include")

set_property(SOURCE cpu/cpu.S PROPERTY LANGUAGE C)

target_sources(arch_amd64 PRIVATE cpu/cpu.S)

add_subdirectory(cpu)
add_subdirectory(lock)

target_include_directories(arch_amd64 PUBLIC "include")
target_include_directories(arch_amd64 PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_include_directories(arch_amd64 BEFORE
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root/include/sys"
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root/include"
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root/include/c++/v1")

add_dependencies(arch_amd64 libcxx_build libcxxabi_build newlib_build)

target_link_directories(arch_amd64 PUBLIC ${CMAKE_BINARY_DIR}/external/third_party_root/lib)

target_link_libraries(arch_amd64 c g m c++ c++abi c++experimental)

target_compile_options(arch_amd64 BEFORE
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