cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE acpi.cc
        PRIVATE ap.cc
        PRIVATE madt.cc
        PRIVATE mcfg.cc
        PRIVATE fadt.cc)