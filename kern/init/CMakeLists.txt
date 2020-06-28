cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
    PRIVATE main.cc multiboot.cc xtors.cc)


# ap_boot

add_executable(ap_boot.elf ${CMAKE_CURRENT_SOURCE_DIR}/ap_boot.S)
set_target_properties(ap_boot.elf PROPERTIES
                            CXX_STANDARD 20
                            CXX_STANDARD_REQUIRED ON
                            CXX_EXTENSIONS ON)

set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/ap_boot.S PROPERTY LANGUAGE C)
target_include_directories(ap_boot.elf PRIVATE "${CMAKE_SOURCE_DIR}/include")

set_target_properties(ap_boot.elf PROPERTIES LINK_DEPENS "${CMAKE_SOURCE_DIR}/config/build/ap_boot.ld")
set_target_properties(ap_boot.elf PROPERTIES LINK_FLAGS "-Wl,--omagic")
set_target_properties(ap_boot.elf PROPERTIES LINK_FLAGS "-Wl,-T ${CMAKE_SOURCE_DIR}/config/build/ap_boot.ld")

target_compile_options(ap_boot.elf BEFORE
    PRIVATE --target=x86_64-pc-linux-elf
    PRIVATE -gdwarf-2
    PRIVATE -fno-pie
    PRIVATE -fno-exceptions
    PRIVATE -fno-rtti
    PRIVATE -ffreestanding
    PRIVATE -nostdlib
    PRIVATE -fno-builtin
    PRIVATE -Wall
    PRIVATE -Wextra
    PRIVATE -march=x86-64
    PRIVATE -mtls-direct-seg-refs
    PRIVATE -mno-sse
    PRIVATE -mcmodel=large
    PRIVATE -mno-red-zone
    PRIVATE -fmodules
    PRIVATE -std=c17)

target_link_options(ap_boot.elf
    PRIVATE -no-pie 
    PRIVATE -nostdlib 
    PRIVATE -ffreestanding 
    PRIVATE -nostartfiles 
    PRIVATE -Wl,--build-id=none)
    

add_custom_command(TARGET ap_boot.elf POST_BUILD
    COMMAND objcopy -S -O binary -j .text $<TARGET_FILE:ap_boot.elf> ${CMAKE_BINARY_DIR}/ap_boot
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)