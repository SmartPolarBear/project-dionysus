cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_executable(monitor
        monitor.cc)

add_custom_command(TARGET monitor POST_BUILD
        COMMAND objdump -S $<TARGET_FILE:monitor> > $<TARGET_FILE_DIR:monitor>/monitor.asm
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )

target_link_libraries(monitor server)

set_target_properties(monitor PROPERTIES LINK_FLAGS "-Wl,-T ${CMAKE_SOURCE_DIR}/config/build/user.ld")

target_include_directories(monitor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(monitor arch_amd64 c++ c++abi c++experimental)

target_compile_options(monitor BEFORE
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


target_link_options(monitor
        PRIVATE -no-pie
        PRIVATE -nostdlib
        PRIVATE -nostartfiles
        PRIVATE -Wl,--build-id=none)