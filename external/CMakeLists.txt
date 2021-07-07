cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(LLVM_PROJ_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/llvm-project/llvm-project)

set(LIBCXX_SRC_DIR ${LLVM_PROJ_SRC_DIR}/libcxx)
set(LIBCXXABI_SRC_DIR ${LLVM_PROJ_SRC_DIR}/libcxxabi)
set(NEWLIB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/newlib/newlib)

set(INSTALL_PREFIX  ${CMAKE_BINARY_DIR}/external/third_party_root)
set(USER_INSTALL_PREFIX  ${CMAKE_BINARY_DIR}/external/third_party_root_user)

add_subdirectory(llvm-project)
add_subdirectory(newlib)
add_subdirectory(ms-gsl)
add_subdirectory(gtest)
add_subdirectory(argparse)
