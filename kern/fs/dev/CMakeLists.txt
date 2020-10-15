cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE ata_devices.cc
        PRIVATE device.cc
        PRIVATE devfs_vnode.cc)
