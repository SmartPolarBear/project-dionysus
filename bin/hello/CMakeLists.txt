cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_executable(hello
        hello.cc)

add_custom_command(TARGET hello POST_BUILD
        COMMAND objdump -S $<TARGET_FILE:hello> > $<TARGET_FILE_DIR:hello>/hello.asm
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )

add_dependencies(hello user)

target_link_libraries(hello user)

target_include_directories(hello PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_compile_options(hello BEFORE
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


target_link_options(hello
        PRIVATE -no-pie
        PRIVATE -nostdlib
        PRIVATE -nostartfiles
        PRIVATE -Wl,--build-id=none
        PRIVATE -Wl,-T ${CMAKE_SOURCE_DIR}/config/build/user.ld)

