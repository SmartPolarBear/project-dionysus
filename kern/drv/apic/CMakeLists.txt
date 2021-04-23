cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE ioapic.cc
        PRIVATE lapic.cc
        PRIVATE apic_trap_handles.cc
        PRIVATE pic.cc
        PRIVATE traps.cc
        PRIVATE timer.cc
        PRIVATE cli_sli.cc
        PRIVATE exception_handles.cc
        PRIVATE msi.cc)