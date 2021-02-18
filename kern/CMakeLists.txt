cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/drv/apic/vectors.S "")

option(KERNEL_ENABLE_DEBUG_FACILITY "Enable kernel debugging facilities" ON)
option(ARCH "architecture library" "AMD64")
option(THREAD_SAFETY_ANALYSIS "CLang's thread safety analysis" ON)

set(TARGET_ARCH_TRIPLE "")
if (${ARCH} STREQUAL "AMD64")
    set(TARGET_ARCH_TRIPLE "x86_64-pc-linux-elf")
    message(STATUS "Target architecture: ${ARCH}. Setting TARGET_ARCH_TRIPLE to ${TARGET_ARCH_TRIPLE}")
else ()
    message(FATAL_ERROR "Invalid architecture option ${ARCH}")
endif ()

add_executable(kernel
        ${CMAKE_CURRENT_SOURCE_DIR}/init/boot.S
        ${CMAKE_CURRENT_SOURCE_DIR}/drv/apic/trapentry_asm.S
        ${CMAKE_CURRENT_SOURCE_DIR}/drv/apic/vectors.S
        user/syscall/syscall_entry.S
        ${CMAKE_CURRENT_SOURCE_DIR}/task/thread/thread.S
        )

add_dependencies(kernel vectors_S)
add_dependencies(kernel ap_boot.elf)

target_sources(kernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/drv/apic/vectors.S)

set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/init/boot.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/drv/apic/trapentry_asm.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/drv/apic/vectors.S PROPERTY LANGUAGE C)
set_property(SOURCE user/syscall/syscall_entry.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/task/thread/thread.S PROPERTY LANGUAGE C)

set_target_properties(kernel PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS ON)

target_compile_definitions(kernel
        PRIVATE -D__ELF__
        PRIVATE -D_LDBL_EQ_DBL
        PRIVATE -D_GNU_SOURCE
        PRIVATE -D_POSIX_TIMERS)

target_compile_options(kernel BEFORE
        PRIVATE --target=${TARGET_ARCH_TRIPLE}
        PRIVATE -mcmodel=large
        PRIVATE -nostdlibinc
        PRIVATE -nostdlib
        PRIVATE -g
        PRIVATE -gdwarf
        PRIVATE -ffreestanding
        PRIVATE -fno-pie
        PRIVATE -fno-exceptions
        PRIVATE -fno-rtti
        PRIVATE -fno-stack-protector
        PRIVATE -fno-builtin
        PRIVATE -mno-sse
        PRIVATE -mno-red-zone
        PRIVATE -msoft-float
        PRIVATE -mtls-direct-seg-refs
        PRIVATE -Wall
        PRIVATE -Wextra
        PRIVATE -Wno-gnu-include-next)

if (${ARCH} STREQUAL "AMD64")
    message(STATUS "Target architecture: ${ARCH}")
    target_compile_options(kernel BEFORE
            PRIVATE -march=x86-64)
endif ()

if (KERNEL_ENABLE_DEBUG_FACILITY)
    message(STATUS "Debug facilities enabled.")
    target_compile_definitions(kernel
            PUBLIC -D_KERNEL_ENABLE_DEBUG_FACILITY)
endif ()

if (THREAD_SAFETY_ANALYSIS)
    message(STATUS "Thread safety analysis enabled.")
    target_compile_options(kernel BEFORE
            PRIVATE -Wthread-safety
            #PRIVATE -Wthread-safety-negative
            PRIVATE -Wthread-safety-beta)

    target_compile_definitions(kernel
            PUBLIC -D_USE_THREAD_SAFETY_ANALYSIS)
endif ()

# FIXME: needs further inspect on PRIVATE -nostdlib which has been removed to enable shared_ptr
target_link_options(kernel
        PRIVATE -fuse-ld=gold
        PRIVATE -ffreestanding
        PRIVATE -nostartfiles
        PRIVATE -static
        PRIVATE -Wl,--build-id=none)

add_dependencies(kernel libcxx_build libcxxabi_build newlib_build GSL)

target_include_directories(kernel BEFORE
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root/include/sys"
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root/include"
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root/include/c++/v1"
        PRIVATE "${CMAKE_SOURCE_DIR}/include"
        PRIVATE "${CMAKE_SOURCE_DIR}/libs/libuser")

target_link_directories(kernel BEFORE PRIVATE ${CMAKE_BINARY_DIR}/external/third_party_root/lib)

target_link_libraries(kernel executable c g m c++abi c++ c++experimental GSL)

set_target_properties(kernel PROPERTIES LINK_DEPENS "${CMAKE_SOURCE_DIR}/config/build/kernel.ld")
set_target_properties(kernel PROPERTIES LINK_FLAGS "-Wl,-T ${CMAKE_SOURCE_DIR}/config/build/kernel.ld")

add_subdirectory(arch)
add_subdirectory(fs)
add_subdirectory(init)
add_subdirectory(libs)
add_subdirectory(memory)
add_subdirectory(task)
add_subdirectory(drv)
add_subdirectory(object)
add_subdirectory(servers)
add_subdirectory(user)

# custom targets

add_custom_command(TARGET kernel POST_BUILD
        COMMAND objdump -S $<TARGET_FILE:kernel> > ${CMAKE_BINARY_DIR}/kernel.asm
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )

add_custom_target(
        vectors_S ALL
        COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/vectors/gvectors.py ${CMAKE_SOURCE_DIR}/config/codegen/gvectors/gvectors.json ${CMAKE_SOURCE_DIR}/kern/drv/apic/vectors.S
        BYPRODUCTS ${CMAKE_SOURCE_DIR}/kern/drv/apic/vectors.S
        COMMENT "Generating vectors"
)