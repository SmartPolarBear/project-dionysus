cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(project-dionysus C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QEMU_CPUS 6)
set(QEMU_MEM 8G)
set(QEMU_DEBUG_PORT 32768)
set(QEMU_GDB -S -gdb tcp::${QEMU_DEBUG_PORT})
set(QEMU_ARGS -d int,cpu_reset -d -no-reboot -drive file=build/disk.img,index=0,media=disk,format=raw -cpu max -smp ${QEMU_CPUS} -m ${QEMU_MEM})

set(QEMU_BIN qemu-system-x86_64)
set(QEMU_BIN_EXE ${QEMU_BIN}.exe)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-2")

# arch libaraies for kernel
add_subdirectory(arch)

# kernel
add_subdirectory(kern)

# other subdirectories
add_subdirectory(bin)
add_subdirectory(libs)

# external projects
add_subdirectory(external)

# custom targets

add_custom_target(disk.img ALL
        COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/diskimg/diskimg.py" update "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/config/build/image.json"
        DEPENDS kernel ap_boot.elf hello ipctest fs monitor
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
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
