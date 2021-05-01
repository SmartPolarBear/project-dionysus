SET(CMAKE_SYSTEM_NAME Generic)                                                                                                                

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(LLD_NAME lld)

set(CMAKE_GLD_LINKER_NAME clang++ CACHE STRING "Name of the clang++ linker")
mark_as_advanced(CMAKE_GLD_LINKER_NAME)

find_program(CMAKE_GLD_LINKER ${CMAKE_GLD_LINKER_NAME})
mark_as_advanced(CMAKE_GLD_LINKER)

if(NOT CMAKE_GLD_LINKER)
        message(FATAL_ERROR "Could not find the linker: ${CMAKE_GLD_LINKER_NAME}")
endif()

set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_GLD_LINKER} <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

# FIXME: use -fuse-ld=${LLD_NAME} here some day
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -z max-page-size=0x1000")

set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)