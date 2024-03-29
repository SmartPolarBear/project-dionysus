cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(project-dionysus C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Python COMPONENTS Interpreter)

if (!Python_FOUND)
    message(ERROR "A python interpreter with higher version than 3.9.0 is required")
else ()
    message(STATUS "Use python executable ${Python_EXECUTABLE}")
endif ()


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

# external projects
add_subdirectory(external)

# other subdirectories
add_subdirectory(libs)
add_subdirectory(bin)

# include
add_subdirectory(include)

# kernel
add_subdirectory(kern)

# tools
add_subdirectory(tools)

# custom targets

add_custom_target(boot_ramdisk ALL
        COMMAND $<TARGET_FILE:mkramdisk> -t ${CMAKE_BINARY_DIR}/bootramdisk -c ${CMAKE_CURRENT_SOURCE_DIR}/config/build/boot_ramdisk.json
        DEPENDS ap_boot.elf hello ipctest
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating boot ramdisk"
        )

add_custom_target(disk.img ALL
        COMMAND ${Python_EXECUTABLE} "${DISK_IMG_PATH}" ${DISK_IMG_ARGS}
        DEPENDS boot_ramdisk mkramdisk kernel ap_boot.elf hello ipctest #monitor fs
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
