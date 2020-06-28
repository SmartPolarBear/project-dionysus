cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

include(ExternalProject)

set(NEWLIB_CFLAGS " -g -nostdlibinc -mno-avx -target x86_64-unknown-none-elf -fPIC -mcmodel=large")
set(NEWLIB_STATIC_LIB ${INSTALL_PREFIX}/lib/libc.a ${INSTALL_PREFIX}/lib/libg.a ${INSTALL_PREFIX}/lib/libm.a)

ExternalProject_Add(
    newlib_build
    PREFIX ${INSTALL_PREFIX} 
    SOURCE_DIR ${NEWLIB_SRC_DIR}
    CONFIGURE_COMMAND ${NEWLIB_SRC_DIR}/newlib/configure CC=clang CC_FOR_BUILD=clang CFLAGS=${NEWLIB_CFLAGS} --target=x86_64-elf --prefix=${INSTALL_PREFIX} --disable-multilib
    BUILD_BYPRODUCTS ${NEWLIB_STATIC_LIB}
)

set(NEWLIB_USER_CFLAGS " -g -nostdlibinc -mno-avx -target x86_64-unknown-pc-elf -fPIC -mcmodel=large")
set(NEWLIB_USER_STATIC_LIB ${USER_INSTALL_PREFIX}/lib/libc.a ${USER_INSTALL_PREFIX}/lib/libg.a ${USER_INSTALL_PREFIX}/lib/libm.a)

ExternalProject_Add(
        newlib_build_user
        PREFIX ${USER_INSTALL_PREFIX}
        SOURCE_DIR ${NEWLIB_SRC_DIR}
        CONFIGURE_COMMAND ${NEWLIB_SRC_DIR}/newlib/configure CC=clang CC_FOR_BUILD=clang CFLAGS=${NEWLIB_USER_CFLAGS} --target=x86_64-elf --prefix=${USER_INSTALL_PREFIX} --disable-multilib
        BUILD_BYPRODUCTS ${NEWLIB_USER_STATIC_LIB}
)
