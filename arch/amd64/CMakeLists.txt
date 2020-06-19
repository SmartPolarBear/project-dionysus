cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_library(arch_amd64 STATIC cpu.S)

target_include_directories(arch_amd64 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

set_property(SOURCE cpu.S PROPERTY LANGUAGE C)

target_sources(arch_amd64 PRIVATE cpu.S)

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