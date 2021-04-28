cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE ioapic.cc
        PRIVATE lapic.cc
        PRIVATE apic_registers.cc
        PRIVATE spurious.cc
        PRIVATE pic.cc
        PRIVATE traps.cc
        PRIVATE timer.cc
        PRIVATE error.cc
        PRIVATE x86_exceptions.cc
        PRIVATE msi.cc)