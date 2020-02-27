cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(project-dionysus C CXX)

set(QEMU_CPUS 6)
set(QEMU_MEM 8G)
set(QEMU_DEBUG_PORT 32768)
set(QEMU_GDB -S -gdb tcp::${QEMU_DEBUG_PORT})
set(QEMU_ARGS -drive file=build/disk.img,index=0,media=disk,format=raw -cpu max -smp ${QEMU_CPUS} -m ${QEMU_MEM})

set(QEMU_BIN qemu-system-x86_64)
set(QEMU_BIN_EXE ${QEMU_BIN}.exe)

# kernel

file(GLOB_RECURSE DRV_SRC RELATIVE ${CMAKE_SOURCE_DIR} CONFIGURE_DEPENDS "drivers/*.cc")
file(GLOB_RECURSE KERN_SRC RELATIVE ${CMAKE_SOURCE_DIR} CONFIGURE_DEPENDS "kern/*.cc")

add_executable(kernel 
    ${CMAKE_SOURCE_DIR}/kern/init/boot.S 
    ${CMAKE_SOURCE_DIR}/drivers/apic/vectors.S
    ${CMAKE_SOURCE_DIR}/drivers/apic/vectors.S # this is on purpose! or cmake will not include it if it doesn't exist and is created first time
    ${CMAKE_SOURCE_DIR}/drivers/apic/trapentry_asm.S
    ${DRV_SRC} 
    ${KERN_SRC})

set_property(SOURCE ${CMAKE_SOURCE_DIR}/kern/init/boot.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_SOURCE_DIR}/drivers/apic/trapentry_asm.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_SOURCE_DIR}/drivers/apic/vectors.S PROPERTY LANGUAGE C)

set_target_properties(kernel PROPERTIES
                            CXX_STANDARD 20
                            CXX_STANDARD_REQUIRED ON
                            CXX_EXTENSIONS ON)

target_include_directories(kernel PRIVATE "${CMAKE_SOURCE_DIR}/include")

set_target_properties(kernel PROPERTIES LINK_DEPENS "${CMAKE_SOURCE_DIR}/config/build/kernel.ld")
set_target_properties(kernel PROPERTIES LINK_FLAGS "-Wl,-T ${CMAKE_SOURCE_DIR}/config/build/kernel.ld")

add_dependencies(kernel ap_boot.elf)


# ap_boot

add_executable(ap_boot.elf ${CMAKE_SOURCE_DIR}/kern/init/ap_boot.S)

set_target_properties(ap_boot.elf PROPERTIES
                            CXX_STANDARD 20
                            CXX_STANDARD_REQUIRED ON
                            CXX_EXTENSIONS ON)

set_property(SOURCE ${CMAKE_SOURCE_DIR}/kern/init/ap_boot.S PROPERTY LANGUAGE C)
target_include_directories(ap_boot.elf PRIVATE "${CMAKE_SOURCE_DIR}/include")

set_target_properties(ap_boot.elf PROPERTIES LINK_DEPENS "${CMAKE_SOURCE_DIR}/config/build/ap_boot.ld")
set_target_properties(ap_boot.elf PROPERTIES LINK_FLAGS "-Wl,--omagic")
set_target_properties(ap_boot.elf PROPERTIES LINK_FLAGS "-Wl,-T ${CMAKE_SOURCE_DIR}/config/build/ap_boot.ld")


# custom targets

add_custom_target(disk.img ALL
        COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/diskimg/diskimg.py" update "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/config/build/hdimage.list" 
        DEPENDS kernel ap_boot.elf
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/drivers/apic/vectors.S
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/vectors/gvectors.py ${CMAKE_SOURCE_DIR}/config/codegen/gvectors/gvectors.json ${CMAKE_SOURCE_DIR}/drivers/apic/vectors.S
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}    
)

add_custom_command(TARGET ap_boot.elf POST_BUILD
    COMMAND objcopy -S -O binary -j .text $<TARGET_FILE:ap_boot.elf> ${CMAKE_BINARY_DIR}/ap_boot
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(qemu
		COMMAND ${QEMU_BIN_EXE} -serial mon:stdio ${QEMU_ARGS}
        DEPENDS kernel ap_boot.elf disk.img
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(qemu-wsl
    COMMAND ${QEMU_BIN} -serial mon:stdio ${QEMU_ARGS}
    DEPENDS kernel ap_boot.elf disk.img
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(qemu-gdb
		COMMAND ${QEMU_BIN} -serial mon:stdio ${QEMU_ARGS} ${QEMU_GDB} &
        DEPENDS kernel ap_boot.elf disk.img
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
