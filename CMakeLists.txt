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

file(WRITE ${CMAKE_SOURCE_DIR}/kern/drv/apic/vectors.S "")

add_executable(kernel 
    ${CMAKE_SOURCE_DIR}/kern/init/boot.S 
    ${CMAKE_SOURCE_DIR}/kern/drv/apic/trapentry_asm.S
    ${CMAKE_SOURCE_DIR}/kern/drv/apic/vectors.S)

add_dependencies(kernel vectors_S)
add_dependencies(kernel ap_boot.elf)

target_sources(kernel PRIVATE ${CMAKE_SOURCE_DIR}/kern/drv/apic/vectors.S)

add_subdirectory(kern)
add_subdirectory(drivers)

set_property(SOURCE ${CMAKE_SOURCE_DIR}/kern/init/boot.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_SOURCE_DIR}/kern/drv/apic/trapentry_asm.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_SOURCE_DIR}/kern/drv/apic/vectors.S PROPERTY LANGUAGE C)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-2")

set_target_properties(kernel PROPERTIES
                            CXX_STANDARD 20
                            CXX_STANDARD_REQUIRED ON
                            CXX_EXTENSIONS ON)

target_compile_options(kernel BEFORE
    PRIVATE --target=x86_64-pc-linux-elf
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
    PRIVATE -fmodules)

target_link_options(kernel
    PRIVATE -no-pie
    PRIVATE -nostdlib
    PRIVATE -ffreestanding
    PRIVATE -nostartfiles
    PRIVATE -Wl,--build-id=none)

target_include_directories(kernel PRIVATE "${CMAKE_SOURCE_DIR}/include")

set_target_properties(kernel PROPERTIES LINK_DEPENS "${CMAKE_SOURCE_DIR}/config/build/kernel.ld")
set_target_properties(kernel PROPERTIES LINK_FLAGS "-Wl,-T ${CMAKE_SOURCE_DIR}/config/build/kernel.ld")

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

# other subdirectories
add_subdirectory(bin)
add_subdirectory(libs)

# custom targets

add_custom_target(
    vectors_S ALL
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/vectors/gvectors.py ${CMAKE_SOURCE_DIR}/config/codegen/gvectors/gvectors.json ${CMAKE_SOURCE_DIR}/kern/drv/apic/vectors.S
    BYPRODUCTS ${CMAKE_SOURCE_DIR}/kern/drv/apic/vectors.S
    COMMENT "Generating vectors"
)

add_custom_target(disk.img ALL
        COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/diskimg/diskimg.py" update "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/config/build/hdimage.list" 
        DEPENDS kernel ap_boot.elf
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_command(TARGET ap_boot.elf POST_BUILD
    COMMAND objcopy -S -O binary -j .text $<TARGET_FILE:ap_boot.elf> ${CMAKE_BINARY_DIR}/ap_boot
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_command(TARGET kernel POST_BUILD
    COMMAND objdump -S $<TARGET_FILE:kernel> > ${CMAKE_BINARY_DIR}/kernel.asm
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
