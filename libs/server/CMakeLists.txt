cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_library(server STATIC entry.S entry_main.cc)

add_subdirectory(syscall)
add_subdirectory(memory)
add_subdirectory(debug)

set_property(SOURCE entry.S PROPERTY LANGUAGE C)

target_include_directories(server PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_include_directories(server PUBLIC "include")
target_include_directories(server PRIVATE "include_private")

target_include_directories(server
        INTERFACE "${CMAKE_BINARY_DIR}/external/third_party_root_user/include/c++/v1"
        INTERFACE "${CMAKE_BINARY_DIR}/external/third_party_root_user/include/sys"
        INTERFACE "${CMAKE_BINARY_DIR}/external/third_party_root_user/include")

target_sources(server PRIVATE entry.S entry_main.cc)

target_link_directories(server BEFORE PRIVATE ${CMAKE_BINARY_DIR}/external/third_party_root_user/lib)

add_dependencies(server libcxx_build_user libcxxabi_build_user newlib_build_user)

target_link_directories(server INTERFACE ${CMAKE_BINARY_DIR}/external/third_party_root_user/lib)

target_link_libraries(server c g m c++ c++abi c++experimental)

#-mno-implicit-float is used to avoid xmm registers, which may cause GPF
target_compile_options(server BEFORE
        PRIVATE --target=x86_64-pc-linux-elf
        PRIVATE -g
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
        PRIVATE -D__ELF__
        PRIVATE -D_LDBL_EQ_DBL
        PRIVATE -D_GNU_SOURCE
        PRIVATE -D_POSIX_TIMERS)



