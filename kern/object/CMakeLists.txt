cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE handle.cc
        PRIVATE object.cc
        PRIVATE object_manager.cc)

