cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_executable(ipctest
        ipctest.cc)

add_custom_command(TARGET ipctest POST_BUILD
        COMMAND objdump -S $<TARGET_FILE:ipctest> > $<TARGET_FILE_DIR:ipctest>/ipctest.asm
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )

target_link_libraries(ipctest libuser)

set_target_properties(ipctest PROPERTIES LINK_FLAGS "-Wl,-T ${CMAKE_SOURCE_DIR}/config/build/user.ld")

target_include_directories(ipctest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(ipctest c++ c++abi c++experimental)

target_compile_options(ipctest BEFORE
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


target_link_options(ipctest
        PRIVATE -no-pie
        PRIVATE -nostdlib
        PRIVATE -nostartfiles
        PRIVATE -Wl,--build-id=none)