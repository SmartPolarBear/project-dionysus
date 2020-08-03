cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_executable(fs
        fs.cc)

add_custom_command(TARGET fs POST_BUILD
        COMMAND objdump -S $<TARGET_FILE:fs> > $<TARGET_FILE_DIR:fs>/fs.asm
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )

target_link_libraries(fs user)

set_target_properties(fs PROPERTIES LINK_FLAGS "-Wl,-T ${CMAKE_SOURCE_DIR}/config/build/user.ld")

target_include_directories(fs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(fs c g m c++ c++abi c++experimental)

target_compile_options(fs BEFORE
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


target_link_options(fs
        PRIVATE -no-pie
        PRIVATE -nostdlib
        PRIVATE -nostartfiles
        PRIVATE -Wl,--build-id=none)


