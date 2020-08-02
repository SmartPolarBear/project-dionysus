cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_executable(hello
        hello.cc)

add_custom_command(TARGET hello POST_BUILD
        COMMAND objdump -S $<TARGET_FILE:hello> > $<TARGET_FILE_DIR:hello>/hello.asm
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )

target_link_libraries(hello user)

set_target_properties(hello PROPERTIES LINK_FLAGS "-Wl,-T ${CMAKE_SOURCE_DIR}/config/build/user.ld")

target_include_directories(hello PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(hello c++ c++abi c++experimental)

target_compile_options(hello BEFORE
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
        PRIVATE -mgeneral-regs-only
        PRIVATE -mno-implicit-float
        PRIVATE -mno-sse
        PRIVATE -mno-avx
        PRIVATE -mcmodel=large
        PRIVATE -mno-red-zone
        PRIVATE -fmodules
        PRIVATE -std=c++2a)


target_link_options(hello
        PRIVATE -no-pie
        PRIVATE -nostdlib
        PRIVATE -nostartfiles
        PRIVATE -Wl,--build-id=none)