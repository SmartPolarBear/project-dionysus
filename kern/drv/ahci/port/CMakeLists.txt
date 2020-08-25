cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE common.cc
        PRIVATE ata.cc
        PRIVATE ata_string.cc
        PRIVATE atapi.cc)