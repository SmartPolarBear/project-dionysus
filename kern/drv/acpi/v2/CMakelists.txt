cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
    PRIVATE acpi_v2.cc xsdt.cc)