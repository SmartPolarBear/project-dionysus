cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_library(user STATIC uentry.S uentry_main.cc)

add_subdirectory(syscall)
add_subdirectory(io)
add_subdirectory(math)
add_subdirectory(memory)

set_property(SOURCE uentry.S PROPERTY LANGUAGE C)

target_include_directories(user PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_include_directories(user PUBLIC "include")
target_include_directories(user PRIVATE "include_private")

target_include_directories(user
        INTERFACE "${CMAKE_BINARY_DIR}/external/third_party_root_user/include/c++/v1"
        INTERFACE "${CMAKE_BINARY_DIR}/external/third_party_root_user/include/sys"
        INTERFACE "${CMAKE_BINARY_DIR}/external/third_party_root_user/include")

target_sources(user PRIVATE uentry.S uentry_main.cc)

target_link_directories(user BEFORE PRIVATE ${CMAKE_BINARY_DIR}/external/third_party_root_user/lib)

add_dependencies(user libcxx_build_user libcxxabi_build_user newlib_build_user)

target_link_directories(user INTERFACE ${CMAKE_BINARY_DIR}/external/third_party_root_user/lib)

target_link_libraries(user c g m c++ c++abi c++experimental)

#-mno-implicit-float is used to avoid xmm registers, which may cause GPF
target_compile_options(user BEFORE
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

