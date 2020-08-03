cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
    PRIVATE process.cc load_code.cc scheduler.cc process_run.S context_switch.S process_ipc.cc)
