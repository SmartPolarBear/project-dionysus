cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_library(user STATIC uentry.S uentry_main.cc)
target_sources(user PRIVATE uentry.S uentry_main.cc)

set_property(SOURCE entry.S PROPERTY LANGUAGE C)

add_subdirectory(syscall)
add_subdirectory(io)
add_subdirectory(math)
add_subdirectory(memory)

#-mno-implicit-float is used to avoid xmm registers, which may cause GPF
target_compile_options(user BEFORE
        PRIVATE -g
        PRIVATE --target=x86_64-pc-linux-elf
        PRIVATE -fno-pie
        PRIVATE -fno-exceptions
        PRIVATE -fno-rtti
        PRIVATE -fno-stack-protector
        PRIVATE -nostdlib
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


target_include_directories(user BEFORE
        PUBLIC "${CMAKE_BINARY_DIR}/external/third_party_root_user/include/sys"
        PUBLIC "${CMAKE_BINARY_DIR}/external/third_party_root_user/include"
        PUBLIC "${CMAKE_BINARY_DIR}/external/third_party_root_user/include/c++/v1"
        PUBLIC "${CMAKE_SOURCE_DIR}/include"
        PUBLIC "include"
        PRIVATE "include_private"
        )

add_dependencies(user libcxx_build_user libcxxabi_build_user newlib_build_user)

target_link_directories(user  BEFORE
        PUBLIC ${CMAKE_BINARY_DIR}/external/third_party_root_user/lib)

target_link_libraries(user PUBLIC c g m c++ c++abi c++experimental)
