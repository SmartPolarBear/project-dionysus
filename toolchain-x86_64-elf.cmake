SET(CMAKE_SYSTEM_NAME Generic)                                                                                                                

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_GLD_LINKER_NAME clang++ CACHE STRING "Name of the Binutils linker")
mark_as_advanced(CMAKE_GLD_LINKER_NAME)

find_program(CMAKE_GLD_LINKER ${CMAKE_GLD_LINKER_NAME})
mark_as_advanced(CMAKE_GLD_LINKER)

if(NOT CMAKE_GLD_LINKER)
        message(FATAL_ERROR "Could not find the linker: ${CMAKE_GLD_LINKER_NAME}")
endif()

set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_GLD_LINKER} <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

set(C_AND_CXX_FLAGS "--target=x86_64-pc-linux-elf -gdwarf-2 -fno-pie -fno-exceptions -fno-rtti -ffreestanding -nostdlib -fno-builtin -Wall -Wextra -march=x86-64 -mtls-direct-seg-refs -mno-sse -mcmodel=large -mno-red-zone -fmodules")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_AND_CXX_FLAGS} -std=c++2a")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_AND_CXX_FLAGS} -std=c17")
set(CMAKE_ASM_FLAGS "${C_AND_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -z max-page-size=0x1000 -no-pie -nostdlib -ffreestanding -nostartfiles -Wl,--build-id=none")

set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)