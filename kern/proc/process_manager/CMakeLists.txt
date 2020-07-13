cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
    PRIVATE proc.cc elf.cc scheduler.cc process_run.S context_switch.S)
