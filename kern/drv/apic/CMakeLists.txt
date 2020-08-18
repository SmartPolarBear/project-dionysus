cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
    PRIVATE ioapic.cc lapic.cc pic.cc traps.cc timer.cc cli_sli.cc exception_handles.cc msi.cc)