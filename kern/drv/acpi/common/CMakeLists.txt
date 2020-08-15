cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
    PRIVATE acpi.cc ap.cc madt.cc mcfg.cc)