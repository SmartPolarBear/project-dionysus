cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/drv/apic/vectors.S "")

add_executable(kernel
        ${CMAKE_CURRENT_SOURCE_DIR}/init/boot.S
        ${CMAKE_CURRENT_SOURCE_DIR}/drv/apic/trapentry_asm.S
        ${CMAKE_CURRENT_SOURCE_DIR}/drv/apic/vectors.S
        ${CMAKE_CURRENT_SOURCE_DIR}/proc/syscall/syscall_entry.S
        ${CMAKE_CURRENT_SOURCE_DIR}/proc/process_manager/process_run.S)

add_dependencies(kernel vectors_S)
add_dependencies(kernel ap_boot.elf)

target_sources(kernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/drv/apic/vectors.S)

set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/init/boot.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/drv/apic/trapentry_asm.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/drv/apic/vectors.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/proc/syscall/syscall_entry.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/proc/process_manager/process_run.S PROPERTY LANGUAGE C)


set_target_properties(kernel PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS ON)

target_compile_options(kernel BEFORE
        PRIVATE -g
        PRIVATE --target=x86_64-pc-linux-elf
        PRIVATE -fno-pie
        PRIVATE -fno-exceptions
        PRIVATE -fno-rtti
        PRIVATE -fno-stack-protector
        PRIVATE -nostdlib
        PRIVATE -ffreestanding
        PRIVATE -fno-builtin
        PRIVATE -Wall
        PRIVATE -Wno-gnu-include-next
        PRIVATE -Wextra
        PRIVATE -march=x86-64
        PRIVATE -mtls-direct-seg-refs
        PRIVATE -mno-sse
        PRIVATE -msoft-float
        PRIVATE -mcmodel=large
        PRIVATE -mno-red-zone
        PRIVATE -nostdlibinc
        PRIVATE -D__ELF__
        PRIVATE -D_LDBL_EQ_DBL
        PRIVATE -D_GNU_SOURCE
        PRIVATE -D_POSIX_TIMERS)

target_link_options(kernel
        PRIVATE -nostdlib
        PRIVATE -ffreestanding
        PRIVATE -nostartfiles
        PRIVATE -static
        PRIVATE -Wl,--build-id=none)

add_dependencies(kernel libcxx_build libcxxabi_build newlib_build)

target_include_directories(kernel BEFORE
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root/include/c++/v1"
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root/include/sys"
        PRIVATE "${CMAKE_BINARY_DIR}/external/third_party_root/include"
        PRIVATE "${CMAKE_SOURCE_DIR}/include"
        PRIVATE "${CMAKE_SOURCE_DIR}/libs/libuser")

target_link_directories(kernel BEFORE PRIVATE ${CMAKE_BINARY_DIR}/external/third_party_root/lib)

target_link_libraries(kernel arch_amd64 c g m c++abi c++ c++experimental)

set_target_properties(kernel PROPERTIES LINK_DEPENS "${CMAKE_SOURCE_DIR}/config/build/kernel.ld")
set_target_properties(kernel PROPERTIES LINK_FLAGS "-Wl,-T ${CMAKE_SOURCE_DIR}/config/build/kernel.ld")

add_subdirectory(init)
add_subdirectory(libs)
add_subdirectory(mm)
add_subdirectory(proc)
add_subdirectory(drv)
add_subdirectory(objm)
add_subdirectory(servers)

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