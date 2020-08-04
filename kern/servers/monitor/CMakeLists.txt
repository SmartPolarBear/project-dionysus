cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_executable(monitor
        monitor.cc)

set_target_properties(monitor PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS ON)

add_custom_command(TARGET monitor POST_BUILD
        COMMAND objdump -S $<TARGET_FILE:monitor> > $<TARGET_FILE_DIR:monitor>/monitor.asm
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )

target_link_libraries(monitor user)

set_target_properties(monitor PROPERTIES LINK_FLAGS "-Wl,-T ${CMAKE_SOURCE_DIR}/config/build/user.ld")

target_include_directories(monitor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(monitor arch_amd64 c g m c++ c++abi c++experimental)

target_compile_options(monitor BEFORE
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
        PRIVATE -fmodules)


target_link_options(monitor
        PRIVATE -no-pie
        PRIVATE -nostdlib
        PRIVATE -nostartfiles
        PRIVATE -Wl,--build-id=none)