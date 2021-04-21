cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

if (BUILD_GTEST)
    message(STATUS "Build a google test library.")
    add_subdirectory(google_test)
else ()
    message(STATUS "Use prebuilt GTest")

endif ()