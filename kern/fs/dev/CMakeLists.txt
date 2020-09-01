cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE dev.cc
        PRIVATE ATABlockDevice.cc)
