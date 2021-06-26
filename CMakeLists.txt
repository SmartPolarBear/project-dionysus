cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(project-dionysus C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(VERSION_DISKIMG "diskimg tool version" "NEW")
set(DISK_IMG_PATH "${CMAKE_SOURCE_DIR}/tools/diskimg/diskimg2.py")
set(DISK_IMG_ARGS "")

option(BUILD_GTEST "build google test instead of using prebuilt ones" ON)

if (${VERSION_DISKIMG} STREQUAL "NEW")
    set(DISK_IMG_PATH "${CMAKE_SOURCE_DIR}/tools/diskimg/diskimg2.py")
    set(DISK_IMG_ARGS update -f disk.img -t ../disk.img -c ../config/build/image.json -d . -m mountpoint -g ../grub.cfg)
    message(STATUS "Use diskimg at ${DISK_IMG_PATH}")
elseif (${VERSION_DISKIMG} STREQUAL "OLD")
    set(DISK_IMG_PATH "${CMAKE_SOURCE_DIR}/tools/diskimg/diskimg.py")
    set(DISK_IMG_ARGS update ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/config/build/image.json)
    message(STATUS "Use diskimg at ${DISK_IMG_PATH}")
else ()
    message(FATAL_ERROR "Invalid diskimg tool version. Either NEW or OLD.")
endif ()

set(QEMU_CPUS 6)
set(QEMU_MEM 8G)
set(QEMU_DEBUG_PORT 32768)
set(QEMU_GDB -S -gdb tcp::${QEMU_DEBUG_PORT})
set(QEMU_ARGS -d int,cpu_reset -d -no-reboot -drive file=build/disk.img,index=0,media=disk,format=raw -cpu max -smp ${QEMU_CPUS} -m ${QEMU_MEM})

set(QEMU_BIN qemu-system-x86_64)
set(QEMU_BIN_EXE ${QEMU_BIN}.exe)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-2")

# other subdirectories
add_subdirectory(libs)
add_subdirectory(bin)

# include
add_subdirectory(include)

# kernel
add_subdirectory(kern)

# external projects
add_subdirectory(external)

# tools
add_subdirectory(tools)

# custom targets

add_custom_target(disk.img ALL
        COMMAND python3 "${DISK_IMG_PATH}" ${DISK_IMG_ARGS}
        DEPENDS mkramdisk kernel ap_boot.elf hello ipctest #monitor fs
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
