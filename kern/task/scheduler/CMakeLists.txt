cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE scheduler.cc
        PRIVATE round_rubin.cc)

