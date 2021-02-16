cmake_minimum_required(VERSION 3.16 FATAL_ERROR)


if (${ARCH} STREQUAL "AMD64")
    add_subdirectory(amd64)
    target_link_libraries(kernel arch_amd64)
else()
    message( FATAL_ERROR "Invalid architecture option ${ARCH}" )
endif ()

