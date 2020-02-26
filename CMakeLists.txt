cmake_minimum_required(VERSION 3.16)

project(project-dionysus C CXX ASM-ATT)

file(GLOB_RECURSE DRV_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "drivers/*.cc" "drivers/*.S")
file(GLOB_RECURSE KERN_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "kern/*.cc")

add_executable(kernel ${CMAKE_CURRENT_SOURCE_DIR}/kern/init/boot.S ${DRV_SRC} ${KERN_SRC})

set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/kern/init/boot.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/drivers/apic/trapentry_asm.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/drivers/apic/vectors.S PROPERTY LANGUAGE C)

set_target_properties(kernel PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON)

target_include_directories(kernel PRIVATE "${CMAKE_SOURCE_DIR}/include")

set_target_properties(kernel PROPERTIES LINK_DEPENS "${CMAKE_SOURCE_DIR}/config/build/kernel.ld")
set_target_properties(kernel PROPERTIES LINK_FLAGS "-Wl,-T ${CMAKE_SOURCE_DIR}/config/build/kernel.ld")

