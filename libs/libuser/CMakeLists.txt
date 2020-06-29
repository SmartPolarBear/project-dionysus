cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_library(libuser STATIC uentry.S uentry_main.cc)

add_subdirectory(syscall)
add_subdirectory(io)

set_property(SOURCE uentry.S PROPERTY LANGUAGE C)

target_include_directories(libuser PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_include_directories(libuser PUBLIC "include")
target_include_directories(libuser PRIVATE "include_private")

target_include_directories(libuser
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root_user/include/c++/v1"
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root_user/include/sys"
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root_user/include")

target_sources(libuser PRIVATE uentry.S uentry_main.cc)

target_link_directories(libuser BEFORE PRIVATE ${CMAKE_BINARY_DIR}/external/third_party_root_user/lib)

add_dependencies(libuser libcxx_build_user libcxxabi_build_user newlib_build_user)

target_link_directories(libuser INTERFACE ${CMAKE_BINARY_DIR}/external/third_party_root_user/lib)

target_link_libraries(libuser c g m c++ c++abi c++experimental)

#-mno-implicit-float is used to avoid xmm registers, which may cause GPF
target_compile_options(libuser BEFORE
        PRIVATE --target=x86_64-pc-linux-elf
        PRIVATE -gdwarf-2
        PRIVATE -fno-pie
        PRIVATE -fno-exceptions
        PRIVATE -fno-rtti
        PRIVATE -nostdlib
        PRIVATE -Wall
        PRIVATE -Wextra
        PRIVATE -march=x86-64
        PRIVATE -mtls-direct-seg-refs
        PRIVATE -mno-sse
        PRIVATE -mno-avx
        PRIVATE -mcmodel=large
        PRIVATE -mno-red-zone
        PRIVATE -fmodules
        PRIVATE -std=c++2a)

