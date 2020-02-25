cmake_minimum_required(VERSION 3.16)
project(project-dionysusm VERSION 1.0.0 LANGUAGES CXX ASM-ATT)

enable_language(ASM-ATT CXX C)

set(triple x86_64-pc-none-elf )

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})
set(CMAKE_ASM-ATT_COMPILER clang++)

set(FLAGS_SHARED "${FLAGS_SHARED} -fno-pie -fno-exceptions -fno-rtti -ffreestanding -nostdlib -fno-builtin -Wall -Wextra")
set(FLAGS_SHARED "${FLAGS_SHARED} -march=x86-64 -mtls-direct-seg-refs -mno-sse -mcmodel=large -mno-red-zone")

set(CMAKE_CXX_FLAGS "${FLAGS_SHARED} -fmodules")
set(CMAKE_ASM_FLAGS "${FLAGS_SHARED}")


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

FILE(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/**.cc ${CMAKE_CURRENT_SOURCE_DIR}/drivers/**.S)
set(SOURCE_FILES ${SOURCE_FILES})

set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/kern/init/boot.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/drivers/apic/trapentry_asm.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/drivers/apic/vectors.S PROPERTY LANGUAGE C)

message(STATUS ${SOURCE_FILES})

add_executable(kernel ${CMAKE_CURRENT_SOURCE_DIR}/kern/init/boot.S ${SOURCE_FILES})

target_link_options(kernel PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/config/build/kernel.ld)
target_link_options(kernel PRIVATE -z max-page-size=0x1000 -Wl,-no-pie)
target_link_options(kernel PRIVATE -nostdlib -ffreestanding)
target_link_options(kernel PRIVATE -nostartfiles -Wl,--build-id=none )


set_property(TARGET kernel PROPERTY CXX_STANDARD 20)

# add_custom_command(TARGET kernel POST_BUILD
#     COMMAND objdump -S kernel > kernel.asm
#     COMMAND objdump -t kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym
# )