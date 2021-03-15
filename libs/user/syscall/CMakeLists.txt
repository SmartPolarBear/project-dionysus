cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(user
        PRIVATE process.cc
        PRIVATE hello.cc
        PRIVATE console.cc
        PRIVATE ipc.cc
        PRIVATE thread.cc)

