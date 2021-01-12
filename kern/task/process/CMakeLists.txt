cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE process.cc
        PRIVATE process_new.cc
        PRIVATE load_code.cc
        PRIVATE scheduler.cc
        PRIVATE process_ipc.cc
        PRIVATE process.cc)
