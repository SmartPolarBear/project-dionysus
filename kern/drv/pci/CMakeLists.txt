cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE pci.cc
        PRIVATE pci_express.cc
        PRIVATE pci_legacy.cc)